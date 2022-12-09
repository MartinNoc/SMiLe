import os
import numpy as np

CASIA_WEBFACE_PATH = "C:\\workspace\\facenet\\CASIA_webface\\"

# loop over images and build list of labels
labels = []
paths = []
directories = [f.path for f in os.scandir(CASIA_WEBFACE_PATH) if f.is_dir()]

for dirname in directories:
    dir = os.fsencode(dirname)
    for file in os.listdir(dir):
        filename = os.fsdecode(file)
        full_path = os.path.join(dirname, filename)
        label = int(os.path.basename(dir))

        labels.append(label)
        paths.append("C:/workspace/facenet/latent_train_data_npy/" + filename[:-4] + ".npy")

with open("C:/workspace/facenet/latent_train_data_npy/labels.npy", "wb") as f:
    np.save(f, np.array(labels, dtype=np.int32))
with open("C:/workspace/facenet/latent_train_data_npy/paths.npy", "wb") as f:
    np.save(f, np.array(paths, dtype=np.str_))