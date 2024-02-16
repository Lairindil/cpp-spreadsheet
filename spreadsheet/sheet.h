#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

#include <vector>

//class CellsComp {
//public:
//    bool operator()(const Position& lhs, const Position& rhs) const {
//        return lhs == rhs;
//    }
//};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetCellPtr(Position pos) const;
    Cell* GetCellPtr(Position pos);

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
};
