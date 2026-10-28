Scriptname Game Hidden
; Arquivo de apoio SO PARA COMPILAR. Nao e distribuido. O jogo usa o script real.
Form Function GetForm(int aiFormID) global native
Form Function GetFormFromFile(int aiFormID, string asFilename) global native
Actor Function GetPlayer() global native
Function DisablePlayerControls(bool abMovement = true, bool abFighting = true, bool abCamSwitch = false, bool abLooking = false, bool abSneaking = false, bool abMenu = true, bool abActivate = true, bool abJournalTabs = false, int aiDisablePOVType = 0) global native
Function EnablePlayerControls(bool abMovement = true, bool abFighting = true, bool abCamSwitch = true, bool abLooking = true, bool abSneaking = true, bool abMenu = true, bool abActivate = true, bool abJournalTabs = true, int aiDisablePOVType = 0) global native
