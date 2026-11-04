Scriptname ObjectReference extends Form Hidden
; Arquivo de apoio SO PARA COMPILAR. Nao e distribuido. O jogo usa o script real.
ObjectReference Function PlaceAtMe(Form akFormToPlace, int aiCount = 1, bool abForcePersist = false, bool abInitiallyDisabled = false) native
Function SetAnimationVariableBool(string arVariableName, bool abNewValue) native
Function SetAnimationVariableFloat(string arVariableName, float afNewValue) native
int Function GetItemCount(Form akItem) native
Function RemoveItem(Form akItemToRemove, int aiCount = 1, bool abSilent = false, ObjectReference akOtherContainer = None) native
