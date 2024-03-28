#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    //изменяем размер таблицы, если позиция выходит за них
    if (sheet_.empty() ||
        pos.row >= static_cast<int>(sheet_.size())) {
        sheet_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        sheet_[pos.row].resize(pos.col + 1);
    }
    //добавляем ячейку
    if (!sheet_[pos.row][pos.col]) {
        sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    sheet_[pos.row][pos.col]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    if (sheet_.empty() || pos.row >= static_cast<int>(sheet_.size()) || pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        return nullptr;
    }
    return sheet_[pos.row][pos.col].get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    //очищаем, если есть значение
    if (GetCell(pos) != nullptr) {
        sheet_[pos.row][pos.col]->Clear();
        if (!sheet_[pos.row][pos.col]->IsReferenced()) {
            sheet_[pos.row][pos.col].reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if (sheet_.empty()) {
        return {0, 0};
    }
    Size printable_size = {0, 0};
    for (int i = static_cast<int>(sheet_.size()) - 1; i >= 0; --i) {
        for (int j = static_cast<int>(sheet_[i].size()) - 1; j >= 0; --j) {
            if (sheet_[i][j] != nullptr) {
                if (i >= printable_size.rows) {
                    printable_size.rows = i + 1;
                }
                if (j >= printable_size.cols) {
                    printable_size.cols = j + 1;
                }
            }
        }
    }
    return printable_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for (int i = 0; i < print_size.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < print_size.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            if (GetCell({i, j}) == nullptr) {
                output << "";
                continue;
            }
            if (std::holds_alternative<std::string>(sheet_[i][j]->GetValue())) {
                output << std::get<std::string>(sheet_[i][j]->GetValue());
                continue;
            }
            if (std::holds_alternative<double>(sheet_[i][j]->GetValue())) {
                output << std::get<double>(sheet_[i][j]->GetValue());
                continue;
            }
            if (std::holds_alternative<FormulaError>(sheet_[i][j]->GetValue())) {
                output << std::get<FormulaError>(sheet_[i][j]->GetValue());
                continue;
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for (int i = 0; i < print_size.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < print_size.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            if (GetCell({i, j}) == nullptr) {
                output << "";
                continue;
            }
            output << sheet_[i][j]->GetText();
            continue;
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
