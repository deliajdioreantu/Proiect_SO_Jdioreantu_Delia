Phase 1
1. Tool AI folosit: Claude.

2. Prompt-uri:
Ia-mi trimis cerinta proiectului si am adaugat:
"Ajuta-ma te rog cu partea de filter si cele doua functii necesare.
Am o structură REPORT cu campurile: id (int), inspectorName (char[50]),
gps (struct cu latitude și longitude ca float), category (char[30]),
severity (int), timestamp (time_t), description (char[100]).
Generează functiile:

int parse_condition(const char *input, char *field, char *op, char *value)
care sparge un string de forma field:operator:value în cele trei parti pentru evaluare conditiei din comanda.

int match_condition(REPORT *r, const char *field, const char *op, const char *value)
care returneaza 1 daca raportul satisface conditia si 0 altfel.
Campuri pot fi: severity, category, inspector, timestamp.
Operatorii pot fi: ==, !=, <, <=, >, >= "

3. Ce a fost generat
-parse_condition:
Functia care folosește strtok() pentru care sparge string-ul dupa ":" si extrage cele trei parti: field, operator si value.
-match_condition:
Functia care verifica pentru fiecare camp din REPORT daca conditia
e indeplinita. Pentru severity si timestamp converteste value la numar cu atoi()/atol(), pentru category si inspector foloseste strcmp().

4. Ce am modificat
In functia parse_condition, a apelat strtok() direct pe parametrul input, lucru care ar fi stricat argv[] pentru conditiile urmatoare.
pentru urmatoarele conditii. Am adaugat si am lucrat pe o copie, ca string ul initial sa ramana nemodificat.

5. Ce am invatat:
Ca tipul time_t necesita atol() nu atoi() pentru conversie.

Phase 2  
 Proiectul nu permite utilizarea funcției signal(), cerandu-se sigaction().  
 Am cerut AI-ului sa imi explice structura sigaction, cum configurez campurile si cum pot preveni erorile atunci cand programul este intrerupt de semnalele SIGUSR1 si SIGINT
 Solutie: Am configurat masca de semnale folosind sigemptyset si am adăugat flag-ul SA_RESTART ca sistemul sa reia automat apelurile intrerupte.
 Ce am învatat: Am invatat ca semnalele pot intrerupe executia normala a programului si ca sigaction ofera un control mult mai bun si sigur
 asupra comportamentului procesului fata de apelul signal. De asemenea, am aflat ca SA_RESTART face ca apelurile de sistem (de ex pause()) sa fie reluate automat
dupa intreruperea de catre un semnal, si ca in interiorul unui handler trebuie folosite doar functii "async-signal-safe",
motiv pentru care am înlocuit printf() cu write().