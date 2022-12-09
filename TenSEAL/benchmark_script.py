import time
import timeit

import tenseal as ts
import numpy as np

def generate_context(n, bits_scale, int_bits):
    context = ts.context(
        ts.SCHEME_TYPE.CKKS,
        poly_modulus_degree=2**n,
        coeff_mod_bit_sizes=[bits_scale + int_bits, bits_scale, bits_scale + int_bits]
    )

    context.global_scale = pow(2, bits_scale)
    context.generate_galois_keys()

    return context

def full_benchmark():
    bits_scale = 29
    logN = np.arange(12,15+1)

    d = {}
    for n in logN:
        t_list = []
        N = 2**n
        start = time.time()
        context = generate_context(n, bits_scale, 10)
        t_list.append(time.time() - start)

        x = np.random.uniform(low=-1, high=1, size=N//2)
        start = time.time()
        x_enc = ts.ckks_vector(context, x)
        t_list.append(time.time() - start)

        w = np.random.uniform(low=-1, high=1, size=N//2)

        start = time.time()
        y = x_enc.dot(w)
        t_list.append(time.time() - start)

        # --------------

        # W = np.random.uniform(low=-1, high=1, size=(N//2, 2**11))

        # start = time.time()
        # y = x_enc @ W
        # t_list.append(time.time() - start)
        t_list.append(0)

        d[n] = t_list
        # ---------------

    print(f"{'logN':>6s} {'context':>11s} {'encrpyt':>11s} {'dot':>11s} {'mvp':>11s}")
    for k, v in d.items():
        context, encrypt, dot, mvp = v
        print(f"{k:>6d} {context:>11.6f} {encrypt:>11.6f} {dot:>11.6f} {mvp:>11.6f}")

def dot_benchmark():
    
    logN = np.arange(12,15+1)
    for n in logN:
        N = 2**n
        context = generate_context(n, 22, 10)

        x = np.random.uniform(low=-1, high=1, size=N//2)
        x_enc = ts.ckks_vector(context, x)

        w = np.random.uniform(low=-1, high=1, size=N//2)

        loops = 1000
        cum_time = 0
        for _ in range(loops):
            start = time.time()
            x_enc.dot(w)
            cum_time += time.time() - start

        print(f"logN = {n:2d}: {cum_time / loops:>11.6f}")
        

        # result = timeit.timeit(x_enc.dot(w), globals=globals(), number=loops)
        # print(f"{n}: {result/loops}")

def mvp_benchmark():
    logN = np.arange(12,13+1)
    times = {}
    for n in logN:
        N = 2**n
        context = generate_context(n, 22, 10)

        x = np.random.uniform(low=-1, high=1, size=N//2)
        x_enc = ts.ckks_vector(context, x)

        W = np.random.uniform(low=-1, high=1, size=(N//2, 2**11))

        loops = 10
        cum_time = 0
        for l in range(loops):
            start = time.time()
            x_enc @ W
            cum_time += time.time() - start

            if cum_time > 3600: break

        times[n] = cum_time / l

    for dim, t in times.items():
        print(f"logN = {dim:2d}: {t:>11.7f}")

if __name__ == '__main__':
    mvp_benchmark()
