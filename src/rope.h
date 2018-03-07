#pragma once
#ifndef ROPE_INCLUDED
#define ROPE_INCLUDED

typedef union Rope *ROPE;

ROPE Rope_createFromString(const char *data, int size);

void Rope_dump(ROPE r, int indent);

#endif
