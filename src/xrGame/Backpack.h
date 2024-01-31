#pragma once

#include "inventory_item_object.h"

// Author: Charsi82

class CBackpack : public CInventoryItemObject
{
private:
    using inherited = CInventoryItemObject;

public:
    float m_additional_weight;

    int m_iInventoryCapacity = 0;
    int				GetInventoryCapacity() const { return m_iInventoryCapacity; }
    CBackpack() : m_additional_weight(0.f) {};
    virtual ~CBackpack() = default;

    virtual void Load(LPCSTR section);

    float AdditionalInventoryWeight();
};