Scriptname Actor extends ObjectReference Hidden
; Arquivo de apoio SO PARA COMPILAR. Nao e distribuido. O jogo usa o script real.
bool Function IsDead() native
Function PushActorAway(Actor akActorToPushFrom, float aiKnockbackForce) native
int Function GetEquippedItemType(int aiHand) native
float Function GetActorValue(string asValueName) native
Function SetActorValue(string asValueName, float afValue) native
Function DamageActorValue(string asValueName, float afDamage) native
Function RestoreActorValue(string asValueName, float afAmount) native
bool Function IsInKillMove() native
bool Function HasSpell(Form akForm) native
bool Function AddSpell(Spell akSpell, bool abVerbose = true) native
int Function GetFactionReaction(Actor akOther) native
