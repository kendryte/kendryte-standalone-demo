# -*- coding: utf-8 -*-
"""
Created on Sat Mar 17 10:38:47 2018

@author: zhangtao
"""

import numpy as np

scale, bias = 0.12349300010531557, -13.528213


softmax_array = np.array((range(-255, 1)))
softmax_array = softmax_array * scale
softmax_array = np.exp(softmax_array)

sigmoid_array = np.array((range(0, 256)))
sigmoid_array = sigmoid_array * scale + bias
sigmoid_array = 1 / (1 + np.exp(-sigmoid_array))

write_buff = 'float scale = {};\nfloat bais = {};\n'.format(scale, bias)
write_buff += 'static const float activate_array_acc[] = {\n'
for index in range(len(sigmoid_array)):
    if index % 4 == 0:
        write_buff += '\t'
    write_buff += '{:.16e}'.format(sigmoid_array[index]) + ','
    if index % 4 == 3:
        write_buff += '\n'
    else:
        write_buff += ' '
write_buff += '};\n\n'
write_buff += 'static const float softmax_acc[] = {\n'
for index in range(len(softmax_array)):
    if index % 4 == 0:
        write_buff += '\t'
    write_buff += '{:.16e}'.format(softmax_array[index]) + ','
    if index % 4 == 3:
        write_buff += '\n'
    else:
        write_buff += ' '
write_buff += '};'

with open(r'region_layer_array.include', 'w') as file:
    file.write(write_buff)

