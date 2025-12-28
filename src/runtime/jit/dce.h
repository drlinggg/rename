#ifndef DCE_H
#define DCE_H

#include "../../system.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"

typedef struct {
    size_t removed_calls;      // Количество удаленных вызовов функций
    size_t removed_instructions; // Общее количество удаленных инструкций
    size_t optimized_functions; // Количество оптимизированных функций
    size_t side_effect_calls;  // Вызовы с побочными эффектами, которые НЕ были удалены
} DCEStats;

// Оптимизация устранения мертвого кода
CodeObj* jit_optimize_dce(CodeObj* code, DCEStats* stats);

#endif // DCE_H
