from config import CONFIG_PIPE
from filter_v0 import process_fens_parallel


def run():
    d = CONFIG_PIPE['v0_data']
    tasks = list(zip(d['input_data'], d['output_data'], d['seed']))
    process_fens_parallel(tasks)


if __name__ == '__main__':
    run()