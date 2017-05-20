extern LANGSPEC bool quantitize_note(const struct Blocks *block, struct Notes *note);

extern LANGSPEC void SCHEME_throw(const char *symbol, const char *message);
extern LANGSPEC const char *SCHEME_get_history(void);
extern LANGSPEC bool SCHEME_mousepress(int button, float x, float y);
extern LANGSPEC bool SCHEME_mousemove(int button, float x, float y);
extern LANGSPEC bool SCHEME_mouserelease(int button, float x, float y);
extern LANGSPEC dyn_t SCHEME_eval(const char *code);
extern LANGSPEC int SCHEME_get_webserver_port(void);
extern LANGSPEC void SCHEME_init1(void);
extern LANGSPEC void SCHEME_init2(void);
