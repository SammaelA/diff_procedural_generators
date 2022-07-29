#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "save_utils/blk.h"
#include "common_utils/utility.h"
#include "cmd_buffers.h"

char _console_buf[4096];
bool add_command_block(Block &b)
{
  int cmd_code = b.get_int("cmd_code", -1);
  if (cmd_code >= 0 && cmd_code < InputCommands::IC_COMMANDS_COUNT)
  {
    // single command
    inputCmdBuffer.push((InputCommands)cmd_code, b);
    return true;
  }
  return false;
}
void read_commands_from_string(std::string &block_str)
{
  BlkManager man;
  Block b;
  man.load_block_from_string(block_str, b);
  bool command_block = add_command_block(b);
  if (!command_block)
  {
    std::string blk_path = block_str;
    if (blk_path != "")
    {
      // list of commands from file
      Block pack;
      man.load_block_from_file(blk_path, pack);
      for (int i = 0; i < pack.size(); i++)
      {
        Block *cmd = pack.get_block(i);
        if (cmd)
          add_command_block(*cmd);
      }
    }
  }
}
void read_from_console_nonblock()
{
  static bool set_nonblock = false;

  if (!set_nonblock)
  {
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    set_nonblock = true;
  }

  int numRead = read(0, _console_buf, 4096);
  if (numRead > 0)
  {
    std::string block_str = std::string(_console_buf);
    read_commands_from_string(block_str);
  }
}