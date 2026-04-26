# Raport de Utilizare AI - Sisteme de Operare Faza 1

## 1. Tool-ul utilizat
Pentru asistența permisă în cadrul acestei faze, am utilizat **Google Gemini**.

## 2. Prompturile oferite
Pentru a respecta cerințele, am folosit două prompturi specifice:

**Prompt 1 (Pentru parsare):**
> "Am o aplicație de sistem în C unde primesc argumente de filtrare de forma 'camp:operator:valoare' (ex: severity:>=:2). Generează-mi te rog funcția `int parse_condition(const char *input, char *field, char *op, char *value);` care să despartă acest șir în cele 3 variabile folosind `strtok`."

**Prompt 2 (Pentru evaluarea condiției):**
> "Am o structură `Report` în C cu următoarele câmpuri relevante: `severity` (int) și `category` (string de 16 caractere). Generează funcția `int match_condition(Report *r, const char *field, const char *op, const char *value);` care compară un raport cu filtrul extras anterior. Ai grijă la tipurile de date: `value` este string, deci pentru `severity` trebuie convertit în număr."

## 3. Ce a generat AI-ul
AI-ul a generat cod funcțional pentru ambele funcții:
* A folosit `strtok(input, ":")` pentru a sparge șirul în cele 3 variabile.
* A folosit o serie lungă de instrucțiuni `if-else` alături de `strcmp` pentru a verifica numele câmpului (field) și apoi alt set de `if-else` pentru operator (op).
* A folosit `atoi(value)` pentru a transforma textul "2" în numărul întreg `2` înainte de comparație.

## 4. Ce am modificat din codul generat și DE CE
Nu am lăsat codul exact cum l-a dat AI-ul, ci am făcut următoarele modificări critice:

1. **Protejarea șirului original (Buffer Copy):**
   * *Problema AI-ului:* Funcția lui aplica `strtok` direct pe parametrul `const char *input`. Funcția `strtok` distruge șirul original (înlocuiește `:` cu caracterul terminator `\0`). Dacă încerci să modifici un string constant sau un argument din `argv` în unele medii de execuție, programul dă *Segmentation Fault*.
   * *Modificarea mea:* Am adăugat un buffer temporar `char temp[256];` în care am copiat șirul cu `strncpy` înainte de a-l parsa.

2. **Simplificarea operatorilor (Modularizare):**
   * *Problema AI-ului:* A scris zeci de linii de cod acoperind combinații inutile (ex: `<` pentru string-uri). 
   * *Modificarea mea:* Am redus funcția `match_condition` strict la ce am nevoie momentan pentru logica aplicației (ex: `>`, `>=`, `==` pentru severitate și `==` pentru categorii), menținând codul curat și ușor de citit.

## 5. Ce am învățat din acest exercițiu
* Am înțeles exact cum funcționează `strtok`: acesta păstrează un pointer intern la ultima poziție (de aceea apelurile ulterioare se fac cu `strtok(NULL, ":")`) și modifică direct zona de memorie a textului pe care îl analizează.
* Am învățat că valorile textuale primite prin linia de comandă (chiar dacă reprezintă cifre) nu pot fi comparate direct cu numere întregi. Funcții utilitare precum `atoi()` (ASCII to Integer) sunt obligatorii pentru a aduce datele la un tip comun, necesar procesorului pentru a face operații logice corecte.