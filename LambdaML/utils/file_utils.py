import os
import numpy as np


def random_files(num_files, path):
    for i in np.arange(num_files):
        w = np.random.rand(2, 3)
        np.savetxt(path + "/" + str(i), w)


def create_file(path):
    """return a new file object ready to write to"""
    print("creating file {}".format(path))
    return open(path, 'w')


def split_file_with_info(src_path, dst_path, num_files):
    line_number = 0

    src_file = open(src_path, "r")
    print("Splitting {} into {} files".format(src_path, num_files))

    dst_file = []
    for i in np.arange(num_files):
        file_name = "{}_{}".format(i, num_files)
        dst_file_name = os.path.join(dst_path, file_name)
        dst_file.append(create_file(dst_file_name))

    for line in src_file:
        file_index = line_number % num_files
        dst_file[file_index].write(line)
        line_number += 1

    for file in dst_file:
        file.close()

def uniform_line_split(src_path, dst_path, num_files, num_lines):
    line_number = 0

    src_file = open(src_path, "r")
    print("Splitting {} into {} files".format(src_path, num_files))

    dst_file = []
    for i in np.arange(num_files):
        file_name = "{}".format(i, num_files)
        dst_file_name = os.path.join(dst_path, file_name)
        dst_file.append(create_file(dst_file_name))

    for line in src_file:
        file_index = line_number % num_files
        dst_file[file_index].write(line)
        line_number += 1
        if line_number >= num_files * num_lines:
            break

    for file in dst_file:
        file.close()


def split_file_with_index(src_path, dst_path, start, end):
    line_number = 0
    num_files = end - start

    src_file = open(src_path, "r")
    print("Splitting {} into {} files".format(src_path, num_files))

    dst_file = []
    for i in np.arange(start, end):
        file_name = str(i)
        dst_file_name = os.path.join(dst_path, file_name)
        dst_file.append(create_file(dst_file_name))

    for line in src_file:
        file_index = line_number % num_files
        dst_file[file_index].write(line)
        line_number += 1

    for file in dst_file:
        file.close()


def create_trigger_file(path, num_files):
    for i in np.arange(num_files):
        file_name = "{}_{}".format(i, num_files)
        dst_file_name = os.path.join(path, file_name)
        f = open(dst_file_name, 'w')
        f.write("test")
        f.close()


def merge_files(src_files, dst_file):
    dst_file = open(dst_file, 'w')
    for src_name in src_files:
        src_file = open(src_name, 'r')
        for line in src_file:
            dst_file.write(line)
        src_file.close()
    dst_file.close()


def split_file_with_info2(src_file, dst_path, num_files):
    line_number = 0

    #src_file = open(src_path, "r")
    print("Splitting the files into {} files".format(num_files))
    dst_file = []
    dst_file_names= []
    for i in np.arange(num_files):
        file_name = "{}_{}".format(i, num_files)
        dst_file.append([])
        dst_file_names.append(file_name)

    for line in src_file:
        file_index = line_number % num_files
        line = line.strip("\n")
        dst_file[file_index].append(line+"\n") 
        line_number += 1
    i = 0
    for file in dst_file:
        put_object(dst_path,dst_file_names[i],bytes(''.join(file), encoding = "utf8"))
        i = i+1


if __name__ == "__main__":
    num_file = 256
    num_line = 1500
    src_file = "/Volumes/Daten/Datasets/higgs/HIGGS1G"
    dst_dir = "/Volumes/Daten/Datasets/higgs/HIGGS1M"
    #split_file_with_info(src_file, dst_dir, num_file)
    uniform_line_split(src_file, dst_dir, num_file, num_line)
