#!/usr/bin/env python3

import os
from os import path
import pathlib
import subprocess
import argparse

def get_filename_without_exy(filename):
    return path.splitext(filename)[0]

def get_filename_from_path(fullpath):
    return path.basename(fullpath)

def get_script_dir():
    return pathlib.Path(__file__).parent.resolve()

def get_project_root_dir():
    return pathlib.Path(get_script_dir()).parent.resolve()

def get_log_file_path():
    return path.join(get_script_dir(), 'auto_run_logs.log')

def prepare_wdir():
    current_dir = get_script_dir()
    files_in_current_dir = [f for f in os.listdir(current_dir) if path.isfile(path.join(current_dir, f))]
    rose_tmp_files_to_delete =[f for f in files_in_current_dir if ('.cpp' in f and 'rose_' == f[0:5])]
    print('Warning:', 'Deleting rose-generated files..')
    print('Warning:', rose_tmp_files_to_delete)
    for file_to_delete in rose_tmp_files_to_delete:
        file_to_rem = pathlib.Path(path.join(current_dir, file_to_delete))
        file_to_rem.unlink()
    print('Info:', 'Successfully removed', len(rose_tmp_files_to_delete), 'rose-generated files')
    log_file_to_rem = pathlib.Path(get_log_file_path())
    if log_file_to_rem.exists():
        log_file_to_rem.unlink()
        print('Info:', 'Successfully removed the log file:', log_file_to_rem)


def run_ap_exe(target_path, j_arg, e_arg, enable_debug):
    args_cmd_str = f'{target_path} -j{j_arg} -e{e_arg}'
    if enable_debug:
        args_cmd_str += ' -d'
    command_list = [
        'make',
        'run_ap',
        f'ARGS={args_cmd_str}'
    ]
    my_env = os.environ.copy()
    my_env['ROSE_PATH'] = '/u/course/ece1754/rose/ROSE_INSTALL'

    print('Info:', 'Launching:', ' '.join(command_list))
    with open(get_log_file_path(), 'a') as f:
        process = subprocess.Popen(command_list, env=my_env, stdout=f, stderr=f)
        process.wait()
    print('Info:', '  ', 'Done')

    generated_filename = 'rose_' + get_filename_from_path(target_path)
    fixed_filename = 'rose_' + str(e_arg) + '_' + str(j_arg) + '_' + get_filename_from_path(target_path)
    generated_dir = get_script_dir()
    os.rename(path.join(generated_dir, generated_filename), path.join(generated_dir, fixed_filename))
    print('Info:', '  ', 'Renamed', generated_filename, 'to', fixed_filename)
    return fixed_filename

def init_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('target', type=str,
                        help='Path to the target program')
    # parser.add_argument('--sum', dest='accumulate', action='store_const',
    #                     const=sum, default=max,
    #                     help='sum the integers (default: find the max)')
    return parser

def main(args):
    prepare_wdir()

    generated_filenames = list()
    e_j_schedule = [(0, 1), (0, 2), (0, 4), (0, 8), (1, 1), (1, 2), (1, 4), (1,8), (2, 1)]
    for e, j in e_j_schedule:
        generated_filenames.append(run_ap_exe(args.target, j, e, True))
    
    print('Info:', 'Done running', len(generated_filenames), 'ap_exe run!')
    print('Info:', '  ', 'Generated files in', get_script_dir())
    for generated_filename in generated_filenames:
        print('Info:','  ', '  ', generated_filename)
    print('Info:', 'Log file:', get_log_file_path())

if __name__ == '__main__':
    parser = init_parser()
    main(parser.parse_args())