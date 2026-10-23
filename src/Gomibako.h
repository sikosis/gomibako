#ifndef GOMIBAKO_H
#define GOMIBAKO_H

#include <SupportDefs.h>

extern const char* const kApplicationSignature;
extern const char* const kDeskbarItemName;
extern const char* const kReplicantClassName;

const uint32 kMessageRemoveFromDeskbar = 'grmv';
const uint32 kMessageRegisterReplicant = 'greg';
const uint32 kMessageTrashState = 'gsta';

#endif
