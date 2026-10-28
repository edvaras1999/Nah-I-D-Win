Scriptname Form Native Hidden
; Arquivo de apoio SO PARA COMPILAR. Nao e distribuido. O jogo usa o script real.
Function RegisterForModEvent(string eventName, string callbackName) native
bool Function RegisterForAnimationEvent(ObjectReference akSender, string asEventName) native
Function RegisterForKey(int keyCode) native
Event OnAnimationEvent(ObjectReference akSource, string asEventName) native
Event OnKeyDown(int keyCode) native
