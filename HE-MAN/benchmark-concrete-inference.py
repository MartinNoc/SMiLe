import numpy as np
from subprocess import run
from time import time


with open("tmp/mnist_benchmark.csv", "w") as f:
    f.write("sample,encrypt,inference,decrypt,y0,y1,y2,y3,y4,y5,y6,y7,y8,y9\n")
    for j in range(1000):
        t0 = time()
        run(["tenseal_inference", "encrypt", "-k", "tmp/key", "-i", f"tmp/mnist_samples/{j}.npy", "-o", f"tmp/mnist_samples/{j}.enc"])
        t1 = time()
        run(["tenseal_inference", "inference", "-m", "models/mnist.onnx", "-k", "tmp/key", "-i", f"tmp/mnist_samples/{j}.enc", "-o", f"tmp/mnist_samples/{j}.out.enc"])
        t2 = time()
        run(["tenseal_inference", "decrypt", "-k", "tmp/key", "-i", f"tmp/mnist_samples/{j}.out.enc", "-o", f"tmp/mnist_samples/{j}.out.npy"])
        t3 = time()
        y = np.load(f"tmp/mnist_samples/{j}.out.npy")
        f.write(f"{j},{t1-t0},{t2-t1},{t3-t2},")
        f.write(",".join([str(v) for v in y]))
        f.write("\n")
        f.flush()
        

