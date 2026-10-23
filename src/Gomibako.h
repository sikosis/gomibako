#ifndef GOMIBAKO_H
#define GOMIBAKO_H

#include <SupportDefs.h>

extern const char* const kApplicationSignature;
extern const char* const kDeskbarItemName;
extern const char* const kReplicantClassName;

const uint32 kMessageRemoveFromDeskbar = 'grmv';
const uint32 kMessageRegisterReplicant = 'greg';
const uint32 kMessageTrashState = 'gsta';
const uint32 kMessageOpenTrash = 'gopn';
const uint32 kMessageEmptyTrash = 'gemp';
const uint32 kMessageRestoreTrash = 'grst';
const uint32 kMessageTrashRefs = 'gref';
const uint32 kMessageCommandFinished = 'gfin';

#endif
