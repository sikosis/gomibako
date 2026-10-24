#ifndef GOMIBAKO_H
#define GOMIBAKO_H

#include <SupportDefs.h>

extern const char* const kApplicationSignature;
extern const char* const kDeskbarItemName;
extern const char* const kReplicantClassName;

const uint32 kMessageQuit = 'gqui';
const uint32 kMessageRegisterReplicant = 'greg';
const uint32 kMessageTrashState = 'gsta';
const uint32 kMessageOpenTrash = 'gopn';
const uint32 kMessageEmptyTrash = 'gemp';
const uint32 kMessageRestoreTrash = 'grst';
const uint32 kMessageTrashRefs = 'gref';
const uint32 kMessageCommandFinished = 'gfin';
const uint32 kMessageShowSettings = 'gset';
const uint32 kMessageActivateSettings = 'gact';
const uint32 kMessageSettingsChanged = 'gchg';
const uint32 kMessageSettingsControl = 'gctl';
const uint32 kMessageSettingsDefaults = 'gdef';
const uint32 kMessageSettingsRevert = 'grev';
const uint32 kMessageScheduleTimer = 'gtim';
const uint32 kMessageWarningTimer = 'gwti';
const uint32 kMessageSkipAutomaticEmpty = 'gskp';
const uint32 kMessageDismissAutoWarning = 'gdaw';

#endif
