import base64

aa = lambda v, k: bytes([d ^ k[i % len(k)] for i, d in enumerate(v)])
bb = lambda v: aa(v, b'ctmp')
gets = lambda s: bb(base64.b64decode(s[::-1])).decode()
getb = lambda s: bytes(bytearray.fromhex(gets(s)))

data = b'=E0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVVQbVUVBtVRVF0WFVFQdR0UA1FRTJkXENFQZdBA'

# Convert bytes to string
data_str = data.decode()

# Remove leading '=' if present (invalid at start)
if data_str.startswith('='):
    data_str = data_str[1:]

# Decode using provided functions
buffer = getb(data_str)

# Print the decoded bytes in hex form
print(buffer.hex())
