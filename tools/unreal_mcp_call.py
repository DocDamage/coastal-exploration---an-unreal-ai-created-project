"""Call the local editor endpoint, checking trial identity before every operation."""
import argparse
import json
from pathlib import Path
import socket
import uuid

from ue58_paths import PROJECT


def request(method, args, timeout=30):
    request_id = uuid.uuid4().hex
    payload = json.dumps({'id': request_id, 'kind': 'call_function',
                          'method': method, 'args': args}).encode() + b'\n'
    if len(payload) > 262144:
        raise ValueError('Editor request exceeds 256 KiB')
    with socket.create_connection(('127.0.0.1', 30020), timeout=timeout) as connection:
        connection.sendall(payload)
        with connection.makefile('rb') as stream:
            raw = stream.readline(4 * 1024 * 1024 + 1)
    if not raw.endswith(b'\n') or len(raw) > 4 * 1024 * 1024:
        raise RuntimeError('Incomplete or oversized editor response; operation may have run')
    response = json.loads(raw)
    if response.get('id') != request_id:
        raise RuntimeError('Editor response ID mismatch')
    if response.get('ok') is not True:
        raise RuntimeError(str(response.get('error', response)))
    return response['result']


def call(method, args=None, timeout=30):
    identity = request('editor.ping', {}, timeout)
    if Path(identity['project']).resolve() != PROJECT.resolve():
        raise RuntimeError('Port 30020 belongs to another project: ' + identity['project'])
    if not str(identity.get('engine', '')).startswith('5.8.'):
        raise RuntimeError('Expected Unreal 5.8 editor')
    return identity if method == 'editor.ping' else request(method, args or {}, timeout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('method', nargs='?', default='editor.ping')
    parser.add_argument('--args', default='{}', help='JSON arguments')
    parser.add_argument('--code-file', type=Path, help='Python file for editor.execute_python')
    parser.add_argument('--timeout', type=float, default=30)
    options = parser.parse_args()
    args = json.loads(options.args)
    if not isinstance(args, dict):
        parser.error('--args must be a JSON object')
    if options.code_file:
        if options.method != 'editor.execute_python':
            parser.error('--code-file requires editor.execute_python')
        filename = str(options.code_file.resolve())
        source = options.code_file.read_text(encoding='utf-8-sig')
        # Authoring scripts resolve sibling tools and evidence from their own path.
        args['code'] = f'__file__ = {filename!r}\nexec(compile({source!r}, __file__, "exec"))'
    print(json.dumps(call(options.method, args, options.timeout), indent=2))


if __name__ == '__main__':
    main()
