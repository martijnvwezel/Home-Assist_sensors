# Prolog - Auto Generated #
from matplotlib import pyplot as plt
import os
import pandas
import numpy as np
import plotly.express as px
import plotly.graph_objects as go


dataset = pandas.read_csv('C:/Users/marti/Nextcloud/Muino/7.projects/10004-watersensor/analyse/20230428-data_1.csv')

data = dataset.sort_values(by=['Program Time [s]'])


def average_sum(lst, window_size):
    res = []
    med = []
    for i in range(len(lst)-window_size+1):
        a = sum(lst[i:i+window_size])/window_size
        b = np.median(lst[i:i+window_size])
        res = res + [int(a)]
        med = med + [int(b)]
    return res, med


class value_info:
    max = 0.0
    min = 100000.0
    average = 0.0
    above_average_bool = 0

    def update_values(self, val):
        if self.max < val:
            self.max = val
            self.average = (self.max + self.min)/2
        if self.min > val:
            self.min = val
            self.average = (self.max + self.min)/2  # only bit below average





def shift_freq(lst):
    data_0 = value_info()
    res = []


    windows_size = 5
    lst, _ = average_sum(lst, windows_size)

    for i in range(len(lst)):
        data_0.update_values(lst[i])
        if data_0.max - data_0.min > 500:
            break

    counter = 0
    bool_array = []
    for i in range(len(lst)):
        temp = data_0.above_average_bool
        bool_array = bool_array + [temp]
        if lst[i] > data_0.average:
            data_0.above_average_bool = 1
        else:
            data_0.above_average_bool = 0

        if temp != data_0.above_average_bool:
            counter = counter + 1




    return (bool_array + [0]*(windows_size-1)), counter

# plot lines
# plt.plot(data['Program Time [s]'], data['data 2'], label="Raw data")

# bool_array_new = [i * 800 for i in shift_freq(data['data 2'])]
# plt.plot(data['Program Time [s]'], bool_array_new, label="Raw data")


freq, counter_0 = shift_freq(data['data 0'])
data['data_0_bool'] = [i * 800 for i in freq]
freq, counter_1 = shift_freq(data['data 1'])
data['data_1_bool']= [i * 800 for i in freq]
freq, counter_2 = shift_freq(data['data 2'])
data['data_2_bool']= [i * 800 for i in freq]


fig = go.Figure()
fig.add_scatter(x=data['Program Time [s]'], y=data['data_0_bool'], mode='lines',  name=str(int(counter_0//2)))
fig.add_scatter(x=data['Program Time [s]'], y=data['data_1_bool'], mode='lines', name=str(int(counter_1//2)))
fig.add_scatter(x=data['Program Time [s]'], y=data['data_2_bool'], mode='lines',  name=str(int(counter_2//2)))
fig.show()

pass
# plt.plot(data['Program Time [s]'][:-(windows_size-1)],
#          aver_sum, label="Average sum")
# plt.plot(data['Program Time [s]'][:-(windows_size-1)], medium, label="Median")

# plt.legend()
# plt.show()

# dataset['data_0_f'], dataset['data_1_med'] = average_sum(data['data 0'], windows_size) + [0] * (windows_size-1)
# dataset['data_1_f'], dataset['data_2_med'] = average_sum(data['data 1'], windows_size) + [0] * (windows_size-1)
# dataset['data_2_f'], dataset['data_3_med'] = average_sum(data['data 2'], windows_size) + [0] * (windows_size-1)

# plt.savefig("la.png", dpi=300)
# dataset.to_csv('C:/Users/marti/Nextcloud/Muino/7.projects/10004-watersensor/analyse/20230428-data_1_adjusted.csv')
