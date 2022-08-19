#!/bin/bash
make -j16 && valgrind --suppressions=./linux_sdl_gl.sup --leak-check=full --show-leak-kinds=all --track-origins=yes ./main leak_test_cmd_pack.blk
