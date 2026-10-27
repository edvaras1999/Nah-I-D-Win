Scriptname PoiseLENative Native Hidden
{ Native functions implemented by PoiseLE.dll. Used only to report to the plugin (diagnostics). }

; Called once when the bridge script starts, so the plugin log shows that Papyrus is alive.
Function ScriptReady() global native

; Called after each stagger event was handled. "packed" is the number received in the event.
Function Ack(int packed) global native
