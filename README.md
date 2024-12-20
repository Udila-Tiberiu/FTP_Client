## Cerinta

Proiectul consta in implementarea protocolului FTP (Fle Transfer Protocol) intr-o aplicatie client-server, respectand standardul in vigoare: https://datatracker.ietf.org/doc/html/rfc959

Aplicatia va fi implementata in limbajul C/C++. Nu sunt permise folosirea altor librarii care nu sunt standard C/C++ sau care deja implementeaza total sau partial protocolul FTP.

Aplicatia (atat client, cat si server) trebuie implementata astfel incat, cel putin urmatoarele functionalitati sa fie functionale pe partea de client:

- conectare / logare
- deconectare / delogare
- listare fisiere
- urcare fisiere
- descarcare fisiere, inclusiv binary (transfer mode) 
- passive

Pentru ca se cere implementarea standardului, partea de client va trebui sa se conecteze la orice alt server FTP care implementeaza standardul, iar partea de server va trebui sa accepte orice alt client FTP care implementeaza standardul.

Accentul in evaluarea implementarii se va pune pe urmatoarele aspecte:
- validarea parametrilor
- validarea input-ului, sanitizare, etc
- evaluarea rezultatelor apelurilor de functii
- utilizarea api-urilor safe/unsafe
- utilizarea api-urilor bounded unde este cazul
- gestionarea corecta a bufferelor
- aspectul estetic al codului
- NU se evalueaza in mod direct eficienta implementarii.

## Comenzi

- ```help```

  **Afisare meniu de ajutor**
#

- ```login <user:STRING> <pass:STRING>```

 **Comenzi FTP executate**
    ```
    USER user
    PASS pass
    ```
#
- ```logout```

    **Comenzi FTP executate:**
    ```
    QUIT
    ```
#
- ```list <path:STRING>```

    **Comenzi FTP executate:**
    ```
    PASV
    LIST path
    ```
#
- ```list```

    **Comenzi FTP executate**
    ```
    PASV
    LIST
    ```
#
- ```put <path:STRING>```

    **Comenzi FTP executate**
    ```
    PASV
    STOR path
    ```

    Pe Data Transfer Port se trimite continutul fisierului si se inchide socketul.
#
- ```get <path:STRING>```

    **Comenzi FTP executate**
    ```
    PASV
    RETR path
    ```
#
- ```binary```

    **Comenzi FTP executate**
    ```
    TYPE I
    ```
#
- ```ascii```

    **Comenzi FTP executate**
    ```
    TYPE A
    ```


## Clientul a fost testat cu ajutorul serverului FTP Xlight.
