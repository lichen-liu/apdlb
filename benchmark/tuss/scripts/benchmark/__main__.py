import argparse

from . import cuda, cpu


def init(parser):
    subparsers = parser.add_subparsers(dest='target', required=True)

    parser_cpu = subparsers.add_parser('cpu')
    cpu.init(parser_cpu)


def main(args):
    if args.target == 'cpu':
        cpu.main(args=args)


parser = argparse.ArgumentParser()
init(parser)
main(parser.parse_args())
