#This program need "memusage" command to measure memory consumption

import time
import subprocess
import sys
import math
import os
import pandas as pd
import numpy as np
import argparse


def execute_with_memusage(cmd):
    print(cmd)
    result = subprocess.run(['memusage'] + cmd, stderr=subprocess.PIPE).stderr.decode('utf-8')

    result = result.split('\n')[1].split(' ')

    heap_total = int(result[5][:-1])
    heap_peak = int(result[8][:-1])
    stack_peak = int(result[11])
    return heap_peak + stack_peak

def find_all_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file[0] == '.':
                continue
            yield os.path.join(root, file)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input','-i', help='input directory name', required=True)
    parser.add_argument('--output','-o', help='output filename', required=True)
    args = parser.parse_args()
    for x in find_all_files(args.input):
        print(x)
    
    alphas = [str(x/100) for x in range(10, 35, 10) if x < 100*(3-math.sqrt(5))/2]
    algos = ['lzabt_ns', 'lzabt', 'lzabt_comp', 'lzd_ns', 'lzd']

    subprocess.run(['scons'])
    output_file = 'data/out/output'
    columns = ['filename','algo', 'alpha', 'speed[Mchars/sec]', 'memory[MB]','ratio[%]']
    df = pd.DataFrame([], columns=columns)
    for input_file in find_all_files(args.input):

        input_file_size = os.path.getsize(input_file)
        filename = os.path.basename(input_file)
        print('processing ' + input_file)

        for algo in algos:
            for alpha in alphas:
                sum_time = 0
                print('processing alpha = ' + alpha)
                if algo == 'lzd' or algo == 'lzd_ns':
                    cmd = ['out/compress','-f',input_file,'-o',output_file,'-a',algo]
                else:
                    cmd = ['out/compress','-f',input_file,'-o',output_file,'-a',algo, '-p', alpha]

                start = time.time()
                subprocess.run(cmd)
                elapsed_time = time.time() - start
                comp_ratio = round(os.path.getsize(output_file)/input_file_size*100, 4)
                speed = round(input_file_size/1024/1024/elapsed_time, 4)
                memory_peak = round(execute_with_memusage(['memusage'] + cmd)/1024/1024, 4)
                if algo == 'lzd' or algo == 'lzd_ns':
                    new_df = pd.DataFrame([[filename,algo,'NaN', speed, memory_peak, comp_ratio]], columns=columns)
                else:
                    new_df = pd.DataFrame([[filename,algo,alpha, speed, memory_peak, comp_ratio]], columns=columns)
                df = df.append(new_df, ignore_index=True)
                if algo == 'lzd' or algo == 'lzd_ns':
                    break
    df.to_csv(args.output, index=False)
