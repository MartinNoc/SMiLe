import numpy as np
import matplotlib.pyplot as plt
import json
from sys import argv


def readJson(filename: str) -> dict:

    output: dict = {}
    
    with open(filename) as f:
        data = json.load(f)

    for tag in data:

        if tag[0] == "t":
            output[tag] = np.array(data[tag]).mean()

        elif tag == "correct":
            output["accuracy"] = sum(data[tag]) / len(data[tag])

        elif "plain" in tag:
            lst = data[tag][1:]
            output[tag] = np.array(lst).mean(axis=(0))

        else:
            output[tag] = np.array(data[tag]).mean(axis=(0))

    return output


def main() -> None:

    if len(argv) != 2:
        print("Give input file as argument")
        exit(0)

    FILENAME = argv[1]
    data = readJson(FILENAME)

    x = list(range(10))
    y1 = data["cipherRes"]
    y2 = data["plainRes"]
    accuracy = data["accuracy"]

    legend = ["encrypted inference", "plain inference"]

    plt.xticks(x)

    plt.suptitle("Infernce for {} labeled images".format(FILENAME.replace("../measurements/", "")[0]))
    plt.title("accuracy={}".format("%.2f" % accuracy))

    plt.step(x, y1, where="mid", color="red")
    plt.step(x, y2, where="mid", color="blue")
    x.reverse()
    x.append(-.5)
    x.reverse()
    x.append(9.5)
    plt.plot(x, np.zeros(len(x)), color="grey", linestyle="dashed")
    plt.legend(legend)
    plt.xlim([0,9])
    plt.xlabel("neuron")
    plt.ylabel("averaged output")

    plt.savefig(FILENAME.replace("../measurements/", "").replace(".json", "") + ".pdf")


if __name__ == "__main__":
    main()

