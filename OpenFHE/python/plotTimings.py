from os import listdir
from evalMnist import readJson
import matplotlib.pyplot as plt
import numpy as np


def main():
    PATH = "../measurements/"
    WIDTH = .5

    dirList = listdir(PATH)

    dirList = [elem for elem in dirList if not "copy" in elem]

    dirList.sort()

    yTicks = [elem.replace("-data.json", "") for elem in dirList]

    yTicks = tuple(yTicks)

    tConv = []
    tRelu1 = []
    tGemm1 = []
    tRelu2 = []
    tGemm2 = []

    for file in dirList:
        data = readJson(PATH + file)
        
        tConv.append(data["tConv"])
        tRelu1.append(data["tRelu2"])
        tGemm1.append(data["tGemm1"])
        tRelu2.append(data["tRelu2"])
        tGemm2.append(data["tGemm2"])

    data = {
            "conv": np.array(tConv),
            "relu 1": np.array(tRelu1),
            "gemm 1": np.array(tGemm1),
            "relu 2": np.array(tRelu2),
            "gemm 2": np.array(tGemm2)
    }

    fig, ax = plt.subplots()
    bottom = np.zeros(10)

    for name, value in data.items():
        plot = ax.bar(yTicks, value, WIDTH, label=name, bottom=bottom)
        bottom += value

    plt.ylabel("average execution time [s]")
    plt.legend()

    fig.set_size_inches(15,15)

    plt.savefig("timings.pdf", dpi=150)

    plt.show()
    


if __name__ == "__main__":
    main()

