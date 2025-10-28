# Root structure
```
.
├── benchmarks
│   ├── factorial.lang
│   └── first_program.lang
├── docs - дока проекта
│   ├── project-structure.md
│   └── specification.md
├── README.md 
└── src 
    ├── compiler
    │   └── todo
    ├── lexer
    │   ├── lexer.c
    │   ├── lexer.h
    │   ├── token.h
    │   └── todo
    ├── parser
    │   ├── ast.h
    │   ├── parser.c
    │   ├── parser.h
    │   └── todo
    └── runtime
        ├── gc
        │   └── todo
        ├── jit
        │   └── todo
        └── vm
            └── todo
```
## lexer
Берет текст и преобразует его в последовательность токенов описанных в src/lexer/token.h, выбрасывает лексические ошибки
## Parser
Берет последовательность токенов описанных в src/lexer/token.h и создает из них AST-дерево, также в этой директории описаны соответствующие экспрешены, литералы, стейтменты которые находятся внутри ast-дерева
## compiler
Получает ast-дерево и создает набор инструкций байт-кода #todo описание
## runtime
Здесь содержатся jit, gc и сама виртуальная машина которая будет прогонять байт-код (наверное, в процессе обсуждения)

