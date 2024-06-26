#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    
    //проверяем наличие циклических зависимостей
    bool IsThereCircularDependency(const Impl& new_impl) const;
    void CashInvalidation(bool flag = false);
    
    std::unique_ptr<Impl> impl_;
    
    Sheet& sheet_;
    std::unordered_set<Cell*> lhs_cells_;
    std::unordered_set<Cell*> rhs_cells_;
};

