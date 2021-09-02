#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "action.h"
#include "actionPlumbing.h"
#include "info.h"

//draws to terminal
//TODO: add option to show git info
//TODO: refactor
//TODO: add colums for line numbers based on dirCount
void draw(t_state * state)
{
  char ** bufferArray = state->bufferArray;
  int * selected = state->selected;
  int dirCount = *state->dirCount;
  char * cwd = state->cwd;
  FILE * tty = state->tty;
  char color_str[255] = {'\0'};
  int color;
  int reletiveNumber;
  int padNum = (int)ceil(log10((double)dirCount + 1));
  char lineNum[255];

  char numberPadding[255];
  fprintf(tty, "\033[J");
  int cursorLocation;
  if(state->topOfSelection)
  {
    cursorLocation = selected[0];
  } else {
    int i = 0;
    for(; selected[i] != -1; i++);
    cursorLocation = selected[i - 1];
  }

  for(int i = getStart(state);
      i < getEnd(state);
      i++)
  {
    color = 37;
    sprintf(color_str, "");
    reletiveNumber = *selected - i > 0 ? *selected - i : (*selected - i) * (0 - 1);
    sprintf(lineNum, "\e[0;90m%*d\e[0m", padNum, reletiveNumber);

    if(isDir(bufferArray[i]))
    {
      color = 34;
      sprintf(color_str, "\e[%dm" , color);
    }

    if(state->topOfSelection)
    {
      if(i == *selected)
        sprintf(lineNum, "\e[30;100m%*d", padNum, i + 1);
    } else {
      int j = 0;
      for(; selected[j] != -1; j++);

      if(i == selected[j - 1])
        sprintf(lineNum, "\e[30;100m%*d", padNum, i + 1);
    }

    for(int j = 0; selected[j] != -1; j++){
      if(i == selected[j])
      {
        sprintf(color_str, "\e[%d;30m" , color + 10);
        break;
      }
    }

    fprintf(tty, "\033[K%s %s%s\e[0m\n\r", lineNum, color_str, bufferArray[i]);
  }
  fprintf(tty, "\033[%dA", getEnd(state) - getStart(state));
}

int canMatch(mode mode, char * combo, struct actionNode * head)
{
  int matchable = 0;
  char * cmpCombo;

  for(int i = 0; combo[i] > 48 && combo[i] < 47; i++);

  while (head)
  {
    if(strlen(combo) <= strlen(head->action->combo) && (head->action->mode & mode) > 0)
    {
      for(int j = 0; j < strlen(combo); j++)
      {
        if (combo[j] != head->action->combo[j])
          break;
        if(j == strlen(combo) - 1)
          matchable = 1;
      }
    }
    head = head->nextNode;
  }
  return matchable;
}

//TODO: insert mode to rename files
struct actionNode * initDefaultMappings()
{
  struct actionNode * commands;

  commands = initList(initAction(NORMAL | VISUAL, "\x1b", escape));
  listQueue(commands, initAction(NORMAL, "\r", enter));
  listQueue(commands, initAction(NORMAL, "/", Search));
  listQueue(commands, initAction(NORMAL, "j", moveDown));
  listQueue(commands, initAction(NORMAL, "k", moveUp));
  listQueue(commands, initAction(NORMAL, "gg", gotoTop));
  listQueue(commands, initAction(NORMAL, "G", gotoBottom));
  listQueue(commands, initAction(NORMAL, "b", backDir));
  listQueue(commands, initAction(NORMAL, " h", toggleHidden));
  listQueue(commands, initAction(NORMAL, "V", selectOne));
  listQueue(commands, initAction(NORMAL | VISUAL, "dd", removeFile));
  listQueue(commands, initAction(NORMAL | VISUAL, "yy", yank));
  listQueue(commands, initAction(NORMAL, "p", put));
  listQueue(commands, initAction(NORMAL, "D", halfPageDown));
  listQueue(commands, initAction(NORMAL, "U", halfPageUp));

  listQueue(commands, initAction(NORMAL, "v", enterVisual));
  listQueue(commands, initAction(VISUAL, "j", visualMoveDown));
  listQueue(commands, initAction(VISUAL, "k", visualMoveUp));
  listQueue(commands, initAction(VISUAL, "o", changeSelectionPos));
  listQueue(commands, initAction(VISUAL, "\r", printSelected));

  return commands;
}

void printAfter(t_state * state, char * msg)
{
    for(int i = 0; i < getEnd(state) - getStart(state); i++)
      fprintf(state->tty, "\n");
    fprintf(state->tty, "%s\n", msg);
    fprintf(state->tty, "\r\033[J\033[%dA", getEnd(state) - getStart(state) + 1);
}

//TODO: add modmasks
int input(t_state * state, struct actionNode * commands)
{
  char tmp[2] = {' ', '\0'};
  char combo[256];
  combo[0] = '\0';
  char countStr[256];
  char msg[256];
  int countInt = 1;

  struct actionNode * commandPointer;
  commandPointer = commands;
  int containsNonNum = 0;
  while(1)
  {
    tmp[0] = getchar();
    if(tmp[0] == 27 && (strlen(combo) > 1 || strlen(countStr))) return 0;

    if(containsNonNum)
    {
      if (tmp[0] >= 48 && tmp[0] <= 57)
      {
        combo[0] = '\0';
        countStr[0] = '\0';
        continue;
      }
    } else {
      if (tmp[0] >= 48 && tmp[0] <= 57)
      {
        strcat(countStr, tmp);
        countInt = strtol(countStr, NULL, 10);
        sprintf(msg, "%s%s", countStr, combo);
        printAfter(state, msg);
        continue;
      }
    }
    containsNonNum = 1;
    strcat(combo, tmp);

    if (!canMatch(state->mode, combo, commandPointer))
    {
      combo[0] = '\0';
      countStr[0] = '\0';
      return 0;
    }
    sprintf(msg, "%s%s", countStr, combo);
    printAfter(state, msg);

    while(commandPointer != NULL)
    {
      if (strcmp(combo, commandPointer->action->combo) == 0 && (commandPointer->action->mode & state->mode) > 0)
      {
        for(int i = 0; i < countInt; i++)
          if(1 == commandPointer->action->function(state))
            return 1;

        countStr[0] = '\0';
        combo[0] = '\0';
        return 0;
      }
      commandPointer = commandPointer->nextNode;
    }
    commandPointer = commands;
  }
  return 0;
}
