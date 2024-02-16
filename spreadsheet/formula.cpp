#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category)
: category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Div0:
            return "#ARITHM!";
        default:
            return "";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
return output << fe.ToString();
}

namespace {

class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(std::move(expression)))
    {
        try {
    } catch (const FormulaException& fe) {
            throw FormulaException("syntax error");
        }
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        
        const std::function<double(Position)> functor = [&sheet](const Position pos)->double {
            if (!pos.IsValid()) throw FormulaError(FormulaError::Category::Ref);

            const auto* cell = sheet.GetCell(pos);
            
            if (!cell) return 0;
            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }
            if (std::holds_alternative<std::string>(cell->GetValue())) {
                auto value = std::get<std::string>(cell->GetValue());
                double result = 0;
//                if (!value.empty()) {
//                    try {
//                        result = std::stod(value);
//                    } catch (const std::exception&) {
//                        throw FormulaError(FormulaError::Category::Value);
//                    }
//                }
                if (!value.empty()) {
                    std::istringstream in(value);
                    if (!(in >> result) || !in.eof()) throw FormulaError(FormulaError::Category::Value);
                }
                return result;
            }
            throw FormulaError(std::get<FormulaError>(cell->GetValue()));
        };
        
        try {
            return ast_.Execute(functor);
        } catch (const FormulaError& fe) {
            return fe;
        }
    }
    
    std::string GetExpression() const override {
        std::ostringstream ss;
        ast_.PrintFormula(ss);
        return ss.str() ;
    }
    
    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid()) cells.push_back(cell);
        }
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        
        return cells;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
         return std::make_unique<Formula>(std::move(expression));
     }
     catch (...) {
         throw FormulaException("");
     }
}
