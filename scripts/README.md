## trainer.py

#### to run, simply call `python trainer.py`

trainer.py is the training algorithm for the neural network embedded within the Racer. 

To use, `trainer.py` ensure you have classified data in `color_data.txt`

Here is the format:

```
W (17447) main: Color values - Red: 1822, Green: 2184, Blue: 1762, Clear: 2008, Color: White
W (9847) main: Color values - Red: 220, Green: 472, Blue: 124, Clear: 1780, Color: Black
W (16447) main: Color values - Red: 488, Green: 748, Blue: 196, Clear: 1620, Color: Red
W (19947) main: Color values - Red: 428, Green: 1368, Blue: 336, Clear: 2148, Color: Green
```

it requires all colors to be present to train. 200 samples of each color seems to be ok.


## controller.py

controller.py is a simple BLE script that accepts keyboard input and relays it to the Racer. Its great for debugging.

#### to run, simply call `python controller.py`