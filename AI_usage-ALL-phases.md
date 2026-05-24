# Raport de Utilizare AI - Sisteme de Operare Fazele 1 și 2

## Faza 1: Fișiere și Filtrări

### 1. Tool-ul utilizat
Pentru asistența permisă în cadrul acestei faze, am utilizat **Google Gemini**.

### 2. Prompturile oferite
Pentru a respecta cerințele, am folosit două prompturi specifice:

**Prompt 1 (Pentru parsare):**
> "Am o aplicație de sistem în C unde primesc argumente de filtrare de forma 'camp:operator:valoare' (ex: severity:>=:2). Generează-mi te rog funcția `int parse_condition(const char *input, char *field, char *op, char *value);` care să despartă acest șir în cele 3 variabile folosind `strtok`."

**Prompt 2 (Pentru evaluarea condiției):**
> "Am o structură `Report` în C cu următoarele câmpuri relevante: `severity` (int) și `category` (string de 16 caractere). Generează funcția `int match_condition(Report *r, const char *field, const char *op, const char *value);` care compară un raport cu filtrul extras anterior. Ai grijă la tipurile de date: `value` este string, deci pentru `severity` trebuie convertit în număr."

### 3. Ce a generat AI-ul
AI-ul a generat cod funcțional pentru ambele funcții:
* A folosit `strtok(input, ":")` pentru a sparge șirul în cele 3 variabile.
* A folosit o serie lungă de instrucțiuni `if-else` alături de `strcmp` pentru a verifica numele câmpului (field) și apoi alt set de `if-else` pentru operator (op).
* A folosit `atoi(value)` pentru a transforma textul "2" în numărul întreg `2` înainte de comparație.

### 4. Ce am modificat din codul generat și DE CE
Nu am lăsat codul exact cum l-a dat AI-ul, ci am făcut următoarele modificări critice:

1. **Protejarea șirului original (Buffer Copy):**
   * *Problema AI-ului:* Funcția lui aplica `strtok` direct pe parametrul `const char *input`. Funcția `strtok` distruge șirul original (înlocuiește `:` cu caracterul terminator `\0`). Dacă încerci să modifici un string constant sau un argument din `argv` în unele medii de execuție, programul dă *Segmentation Fault*.
   * *Modificarea mea:* Am adăugat un buffer temporar `char temp[256];` în care am copiat șirul cu `strncpy` înainte de a-l parsa.

2. **Simplificarea operatorilor (Modularizare):**
   * *Problema AI-ului:* A scris zeci de linii de cod acoperind combinații inutile (ex: `<` pentru string-uri). 
   * *Modificarea mea:* Am redus funcția `match_condition` strict la ce am nevoie momentan pentru logica aplicației (ex: `>`, `>=`, `==` pentru severitate și `==` pentru categorii), menținând codul curat și ușor de citit.

### 5. Ce am învățat din acest exercițiu
* Am înțeles exact cum funcționează `strtok`: acesta păstrează un pointer intern la ultima poziție (de aceea apelurile ulterioare se fac cu `strtok(NULL, ":")`) și modifică direct zona de memorie a textului pe care îl analizează.
* Am învățat că valorile textuale primite prin linia de comandă (chiar dacă reprezintă cifre) nu pot fi comparate direct cu numere întregi. Funcții utilitare precum `atoi()` (ASCII to Integer) sunt obligatorii pentru a aduce datele la un tip comun, necesar procesorului pentru a face operații logice corecte.

---

## Faza 2: Procese și Semnale

### 1. Tool-ul utilizat
Am utilizat **Google Gemini**.

### 2. Prompturile oferite
Pentru rezolvarea problemelor specifice din Faza 2, am folosit următoarele cerințe pentru documentare:

**Prompt 1 (Pentru semnale):**
> "Oferă-mi un exemplu de utilizare a funcției `sigaction` în C pentru a intercepta semnalele `SIGUSR1` și `SIGINT`, respectând restricția de a nu folosi funcția clasică `signal()`."

**Prompt 2 (Pentru procese):**
> "Cum pot rula comanda externă `rm -rf <folder>` dintr-un proces copil creat cu `fork()`, și cum folosesc `waitpid` în procesul părinte pentru a aștepta finalizarea ștergerii?"

### 3. Ce a generat AI-ul
* Exemple de sintaxă pentru inițializarea structurii `sigaction` și asocierea unui handler.
* Un șablon cu `fork()` urmat de execuția lui `execlp` pe ramura fiului (`pid == 0`) și `waitpid` pe ramura părintelui.

### 4. Ce am modificat din codul generat și DE CE
1. **Ajustarea argumentelor pentru `execlp`:** * AI-ul a generat exemple generice. A trebuit să adaptez funcția la formatul exact cerut de comanda `rm`. Din cauza unor erori de argumente nerecunoscute în timpul testării (`unrecognized option`), am ajustat forma la `execlp("rm", "rm", "-rf", district, NULL);` pentru a pasa corect numele comenzii și opțiunile.
2. **Siguranța în interiorul handlerelor de semnal:**
   * Am modificat modul de printare generat de AI din `printf` în apelul de sistem `write(STDOUT_FILENO, ...)`. Funcția `printf` nu este recomandată în handlere de semnal (nu este async-signal-safe).
3. **Logica de ștergere a symlink-ului:**
   * Am asigurat ordinea corectă de ștergere, integrând apelul `unlink` pentru symlink în ramura `else` (după `waitpid`), alături de afișarea mesajelor de succes/eroare adaptate pentru rolul de manager.

### 5. Ce am învățat din acest exercițiu
* Am învățat utilitatea funcției `waitpid` pentru sincronizarea proceselor; fără ea, procesul părinte s-ar încheia înainte de ștergerea fișierelor de către procesul copil, generând procese orfane/zombie.
* Am înțeles limitările funcțiilor I/O de nivel înalt (precum `printf`) în contextul întreruperilor asincrone (semnale) și nevoia utilizării apelurilor POSIX de nivel scăzut (`write`, `read`, `open`).