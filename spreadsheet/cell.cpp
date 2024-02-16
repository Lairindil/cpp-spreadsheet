#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

class Cell::Impl {
public:
    
    virtual ~Impl() = default;
    virtual std::string GetText() const = 0;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }
    virtual bool IsCacheValid() const {
        return true;
    }
    virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl {
public:
    std::string GetText() const override {
        return "";
    }
    CellInterface::Value GetValue() const override {
        return "";
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
    : text_(std::move(text)) {
        if (text_.empty()) {
            throw std::logic_error("");
        }
    }
    
    std::string GetText() const override {
        return text_;
    }
    
    Value GetValue() const override{
        return (text_[0] == ESCAPE_SIGN) ? text_.substr(1) : text_;
    }
    
private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(const SheetInterface& sheet, std::string str)
    : sheet_(sheet) {
        
        if (str.empty() || str[0] != FORMULA_SIGN) {
            throw std::logic_error("");
        }
        formula_ptr_ = ParseFormula(str.substr(1));
    }
    
    std::string GetText() const override {
        return FORMULA_SIGN + formula_ptr_->GetExpression();;
    }
    
    CellInterface::Value GetValue() const override {
        CellInterface::Value cell_value;
        if(!cache_) {
            cache_ = formula_ptr_->Evaluate(sheet_);
        }
        
        auto value = formula_ptr_->Evaluate(sheet_);
        
        if (std::holds_alternative<double>(value)) {
            cell_value = std::get<double>(value);
        }
        
        if (std::holds_alternative<FormulaError>(value)) {
            cell_value = std::get<FormulaError>(value);
        }
        return cell_value;
    }
    
    bool IsCacheValid() const override {
        return cache_.has_value();
    }

    void InvalidateCache() override {
        cache_.reset();
    }
    
    std::vector<Position> GetReferencedCells() const override {
        return formula_ptr_->GetReferencedCells();
    }
    
private:
    std::unique_ptr<FormulaInterface> formula_ptr_;
    const SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

bool Cell::IsThereCircularDependency(const Impl& new_impl) const {
    if (new_impl.GetReferencedCells().empty()) {
        return false;
    }
    
    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : new_impl.GetReferencedCells()) {
        referenced.insert(sheet_.GetCellPtr(pos));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> to_visit;
    to_visit.push(this);
    
    while (!to_visit.empty()) {
            const Cell* curr_cell = to_visit.top();
            to_visit.pop();
            visited.insert(curr_cell);

            if (referenced.find(curr_cell) != referenced.end()) {
                return true;
            }
            
            if (dynamic_cast<const Impl*>(curr_cell) == &new_impl) {
                // Сразу безусловный выход из цикла
                return false;
            }

            for (const auto& dependent_cell : curr_cell->rhs_cells_) {
                if (visited.find(dependent_cell) == visited.end()) {
                    to_visit.push(dependent_cell);
                }
            }
    }
    return false;
}

void Cell::CashInvalidation(bool flag) {
    if (impl_->IsCacheValid() || flag) {
        impl_->InvalidateCache();
        for (Cell* dependent_cell : lhs_cells_) {
            dependent_cell->CashInvalidation();
        }
    }
}

Cell::Cell(Sheet& sheet)
: impl_(std::make_unique<EmptyImpl>())
, sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    } else if ((text[0] == FORMULA_SIGN) && (text.length() > 1)) {
        impl = std::make_unique<FormulaImpl>(sheet_, std::move(text));
    } else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }

    if (IsThereCircularDependency(*impl)) {
        throw CircularDependencyException("");
    }
    
    impl_ = std::move(impl);

    for (Cell* curr_cell : rhs_cells_) {
     curr_cell->lhs_cells_.erase(this);
    }

    rhs_cells_.clear();

    for (const auto& pos : impl_->GetReferencedCells()) {
     Cell* curr_cell = sheet_.GetCellPtr(pos);
     if (!curr_cell) {
         sheet_.SetCell(pos, "");
         curr_cell = sheet_.GetCellPtr(pos);
     }
     rhs_cells_.insert(curr_cell);
     curr_cell->lhs_cells_.insert(this);
    }

    CashInvalidation(true);
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !lhs_cells_.empty();
}
