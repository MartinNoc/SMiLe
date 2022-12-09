import torch
from onnx2torch import convert

path = "mobile_face_net.onnx"
model = convert(path)

print(model)

CUTOFF_COUNT = 8

from torch import nn

child_list = list(model.children())
print(model)

# Approach: First loop through nodes and find output node
# then loop to find conv_47 and replace all uses of it with output, as shown in https://github.com/pytorch/examples/blob/main/fx/replace_op.py
# this MIGHT already be sufficient, otherwise we need to find how to cut off the nodes following conv_47
for n in model.graph.nodes:
    print(n.target)
    if n.target == "output":
        output = n
    elif n.target == "Add_11":
        new_last = n
    elif n.target == "Conv_47":
        cutoff = n
    elif n.target == "Div_0":
        old_last = n

output.replace_input_with(old_last, new_last)
model.recompile()

print(model)

import os
import cv2
import numpy as np
import torchvision.transforms as transforms
import pickle

CASIA_WEBFACE_PATH = "C:\\workspace\\facenet\\CASIA_webface\\"

# use head to encode input images into latent space
directories = [f.path for f in os.scandir(CASIA_WEBFACE_PATH) if f.is_dir()]

for dirname in directories:
    dir = os.fsencode(dirname)
    for file in os.listdir(dir):
        # load image and label
        filename = os.fsdecode(file)

        full_path = os.path.join(dirname, filename)

        label = int(os.path.basename(dir))
        image = cv2.imread(full_path)
        if len(image.shape) == 2:
            image = np.stack([image] * 3, 2)

        # transform image from 0..255 to -1..1
        transform = transforms.Compose(
            [transforms.ToTensor(), 
            transforms.Normalize(mean=(0.5, 0.5, 0.5), std=(0.5, 0.5, 0.5))]
        )
        image = transform(image)
        image = torch.unsqueeze(image, 0)

        # pass through network head
        latent = model(image)
        latent = latent.cpu().detach().numpy()
        
        # write file out
        with open("C:/workspace/facenet/latent_train_data_npy/" + filename[:-4] + ".npy", "wb") as f:
            np.save(f, latent)
        