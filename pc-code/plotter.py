import sys
import matplotlib.pyplot as plt
import pandas as pd

if len(sys.argv) != 2:
    print("Incorrect number of arguments");
    exit()
    
df = pd.read_csv(sys.argv[1])
df.plot(x='Frequency', y='Amplitude', logx=True)
plt.grid(True)
plt.show()