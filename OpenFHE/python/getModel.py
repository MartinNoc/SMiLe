import onnx
import json
from onnx import numpy_helper


def main():
    m = onnx.load("../model/cryptonets_calibrated.onnx")
    # weights sind in den initializer
    # index und name der initializer
    for index, i in enumerate(m.graph.initializer):
        print(index, i.name)
    w = numpy_helper.to_array(m.graph.initializer[0])
    print(w)

    # grad der approximation
    m.metadata_props[0] # => key: "relu_mode", value: "deg3"
    # lower und upper bounds
    d = json.loads(m.metadata_props[1].value)
    for k, v in d.items():
        print(k,v)


if __name__ == "__main__":
    main()
