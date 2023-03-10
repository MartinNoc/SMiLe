import numpy as np 
import torchvision.transforms as transforms
from torch.utils.data import DataLoader
from torchvision import datasets


PATH = "../img/"


def get_data(data_shape: tuple = (32, 32)) -> tuple:
    transformation = transforms.Compose(
        [transforms.Resize(data_shape), transforms.ToTensor()]
    )
    data_dir = PATH
    test_data = datasets.MNIST(
        data_dir, train=False, download=True, transform=transformation
    )
    test_loader = DataLoader(test_data, batch_size=len(test_data))
    test_dataset_array = next(iter(test_loader))[0].numpy()
    data = test_dataset_array.reshape(10000, 1, 32, 32).astype(np.float32)

    labels = test_data.targets.numpy()

    return (data, labels)


def main() -> None:
    data, labels = get_data()
    
    counters = [0 for _i in range(10)]

    for img, label in zip(data, labels):
        counters[label] += 1
        img.tofile(PATH + str(label) + "-" + str(counters[label]) + ".csv", sep=";", format="%10.5f")

        if sum(counters) == 100:
            break


if __name__ == "__main__":
    main()
