#ifndef PDCOVERLAYPAINTER_H
#define PDCOVERLAYPAINTER_H

#include "PdcTypes.h"

#include <QRect>

class QPainter;

class PdcOverlayPainter
{
public:
    static void paint(QPainter *painter, const QRect &rect, const PdcState &state);
};

#endif // PDCOVERLAYPAINTER_H
