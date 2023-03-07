import numpy as np
from sys import argv


def main():
    if len(argv) != 2:
        print("Give filename as argument.")
        exit(0)

    filename = argv[1]

    img = np.load(filename)
    img = img[0][0]
    img = list(img)

    zeros = [0 for _i in range(28)]

    while len(img) != 32:
        img.append(zeros)

    img = list(np.array(img).transpose())

    zeros = [0 for _i in range(32)]

    while len(img) != 32:
        img.append(zeros)

    img = np.array(img).transpose()

    img.tofile(filename.replace(".npy", ".csv"), sep=";", format="%10.5f")
    np.save(filename, img)


if __name__ == "__main__":
    main()
