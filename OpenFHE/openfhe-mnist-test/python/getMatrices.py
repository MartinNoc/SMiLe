import numpy as np
from os import listdir
import pandas as pd


DIR = "../model/"


def main():
    files = listdir(DIR)

    files = [elem for elem in files if ".npy" in elem]
    
    for file in files:
        print("Opening " + file + ":")

        arr = np.load(DIR + file)
    
        pd.DataFrame(arr).to_csv(DIR + file.replace(".npy", ".csv"), header=None) 


if __name__ == "__main__":
    main()
