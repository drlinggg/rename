# Root structure
```
.
├── benchmarks
│   ├── factorial.lang
│   └── first_program.lang
├── docs
│   ├── project-structure.md
│   ├── bytecode.md
│   ├── vm.md
│   └── specification.md
├── README.md 
└── src 
    ├── compiler
    │   └── todo
    ├── lexer
    │   ├── lexer.c
    │   ├── lexer.h
    │   ├── token.h
    ├── parser
    │   ├── parser.c
    │   ├── parser.h
    ├── AST
    │   ├── ast.c
    │   └── ast.h
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
data: filestream -> lexer -> Token* token_array
## AST
Эта реализация содержит AST узлы которыми можно представить любой набор кода на языке, они итерируемы от корня к листьям и представляют собой полное описание всего происходящего в коде
## Parser
Берет последовательность токенов описанных в src/lexer/token.h и создает из них AST-дерево
data: Token* token_array -> parser -> ast_node* tree
## compiler
Получает ast-дерево и создает набор инструкций байт-кода
data: ast_node* tree -> compiler -> bytecode* array | binary_file
## runtime
...
