import random


# Generate all valid 5-digit decimal ASCII strings from 10000 to 99999
def encode_ascii_digits(n):
    return (ord(str(n)[0]) << 32) | (ord(str(n)[1]) << 24) | (ord(str(n)[2]) << 16) | (ord(str(n)[3]) << 8) | ord(
        str(n)[4])


valid_keys = [encode_ascii_digits(n) for n in range(10000, 100000)]


# Try to find a magic number that produces a perfect hash with no collisions using shift & mask
def find_magic_and_shift(table_bits=20):
    table_size = 1 << table_bits
    max_shift = 64 - table_bits

    for _ in range(1000000):
        magic = random.getrandbits(64) | 1  # ensure odd
        for shift in range(max_shift + 1):
            seen = set()
            collision = False
            for key in valid_keys:
                index = ((key * magic) >> shift) & (table_size - 1)
                if index in seen:
                    collision = True
                    break
                seen.add(index)
            if not collision:
                return magic, shift, table_size
    return None, None, None


print(find_magic_and_shift(18))
