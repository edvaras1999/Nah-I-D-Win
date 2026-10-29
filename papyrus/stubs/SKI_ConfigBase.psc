Scriptname SKI_ConfigBase extends SKI_QuestBase Native Hidden
; Arquivo de apoio SO PARA COMPILAR (o SkyUI real e que corre no jogo). Copia das assinaturas usadas do SDK do SkyUI.
string Property ModName Auto
string[] Property Pages Auto
string Property CurrentPage Auto
int Property TOP_TO_BOTTOM = 0 AutoReadOnly
int Property LEFT_TO_RIGHT = 1 AutoReadOnly
Event OnConfigInit() native
Event OnVersionUpdate(int a_version) native
Event OnPageReset(string a_page) native
Event OnOptionSelect(int a_option) native
Event OnOptionDefault(int a_option) native
Event OnOptionHighlight(int a_option) native
Event OnOptionSliderOpen(int a_option) native
Event OnOptionSliderAccept(int a_option, float a_value) native
int Function GetVersion() native
int Function AddHeaderOption(string a_text, int a_flags = 0) native
int Function AddToggleOption(string a_text, bool a_checked, int a_flags = 0) native
int Function AddSliderOption(string a_text, float a_value, string a_formatString = "{0}", int a_flags = 0) native
Function SetCursorFillMode(int a_fillMode) native
Function SetToggleOptionValue(int a_option, bool a_checked, bool a_noUpdate = false) native
Function SetSliderOptionValue(int a_option, float a_value, string a_formatString = "{0}", bool a_noUpdate = false) native
Function SetSliderDialogStartValue(float a_value) native
Function SetSliderDialogDefaultValue(float a_value) native
Function SetSliderDialogRange(float a_minValue, float a_maxValue) native
Function SetSliderDialogInterval(float a_value) native
Function SetInfoText(string a_text) native
Function ForcePageReset() native
Function LoadCustomContent(string a_source, float a_x = 0.0, float a_y = 0.0) native
Function UnloadCustomContent() native
int Function AddEmptyOption() native
Event OnConfigOpen() native
