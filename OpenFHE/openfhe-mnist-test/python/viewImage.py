import numpy as np
from sys import argv
import matplotlib.pyplot as plt


def main():
    if len(argv) != 2:
        print("Give image file as argument.")
        exit(0)

    filename = argv[1]
    
    img = np.load(filename)
    plt.imshow(img[0][0], interpolation="nearest")
    plt.show()


if __name__ == "__main__":
    main()
    