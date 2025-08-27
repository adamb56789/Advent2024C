data = [[*map(int, l.split())] for l in open('../input/2.txt')]

def good(d, s=0):
    for i in range(len(d)-1):
        if not 1 <= d[i]-d[i+1] <= 3:
            return s and any(good(d[j-1:j] + d[j+1:]) for j in (i,i+1))
    return True

print(sum(good(d, 1) or good(d[::-1], 1) for d in data))