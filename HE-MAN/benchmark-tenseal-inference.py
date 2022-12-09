import numpy as np
from subprocess import run
from time import time


with open("tmp/evaluation/mnist_lenet_small/benchmark.csv", "w") as f:
    f.write("sample,encrypt,inference,decrypt,y0,y1,y2,y3,y4,y5,y6,y7,y8,y9\n")
    for j in range(1):
        t0 = time()
        run(["tenseal_inference", "encrypt", "-k", "tmp/test-n32k/key", "-i", f"tmp/mnist_32x32_tenseal/{j}.npy", "-o", f"tmp/mnist_32x32_tenseal/{j}.enc"])
        t1 = time()
        run(["tenseal_inference", "inference", "-m", "tmp/lenet-5_square.onnx", "-k", "tmp/test-n32k/key.pub", "-i", f"tmp/mnist_32x32_tenseal/{j}.enc", "-o", f"tmp/mnist_32x32_tenseal/{j}.out.enc"])
        t2 = time()
        run(["tenseal_inference", "decrypt", "-k", "tmp/test-n32k/key", "-i", f"tmp/mnist_32x32_tenseal/{j}.out.enc", "-o", f"tmp/mnist_32x32_tenseal/{j}.out.npy"])
        t3 = time()
        y = np.load(f"tmp/mnist_32x32_tenseal/{j}.out.npy")
        f.write(f"{j},{t1-t0},{t2-t1},{t3-t2},")
        f.write(",".join([str(v) for v in y]))
        f.write("\n")
        f.flush()
        

