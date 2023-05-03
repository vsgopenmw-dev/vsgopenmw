#ifndef OPENMW_COMPONENTS_MISC_COORDINATECONVERTER_H
#define OPENMW_COMPONENTS_MISC_COORDINATECONVERTER_H

#include <components/esm/esmbridge.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadpgrd.hpp>
//#include <components/esm4/loadcell.hpp>

namespace Misc
{
    /// \brief convert coordinates between world and local cell
    class CoordinateConverter
    {
    public:
        CoordinateConverter(bool exterior, int cellX, int cellY)
            : mCellX(exterior ? cellX * ESM::Land::REAL_SIZE : 0)
            , mCellY(exterior ? cellY * ESM::Land::REAL_SIZE : 0)
        {
        }

        /*
        explicit CoordinateConverter(const ESM::CellVariant& cell)
            : CoordinateConverter(cell.isEsm4() ? cell.getEsm4().isExterior() : cell.getEsm3().isExterior(),
                cell.isEsm4() ? cell.getEsm4().getGridX() : cell.getEsm3().getGridX(),
                cell.isEsm4() ? cell.getEsm4().getGridY() : cell.getEsm3().getGridY())
        {
        }
        */
        explicit CoordinateConverter(const ESM::CellVariant& cell)
            : CoordinateConverter(cell.getEsm3().isExterior(),
                cell.getEsm3().getGridX(),
                cell.getEsm3().getGridY())
        {
        }
 
        explicit CoordinateConverter(const ESM::Cell* cell)
            : CoordinateConverter(cell->isExterior(), cell->getGridX(), cell->getGridY())
        {
        }

        /// in-place conversion from local to world
        void toWorld(ESM::Pathgrid::Point& point) const
        {
            point.mX += mCellX;
            point.mY += mCellY;
        }

        /// in-place conversion from world to local
        void toLocal(ESM::Pathgrid::Point& point) const
        {
            point.mX -= mCellX;
            point.mY -= mCellY;
        }

        ESM::Pathgrid::Point toWorldPoint(ESM::Pathgrid::Point point) const
        {
            toWorld(point);
            return point;
        }

        ESM::Pathgrid::Point toLocalPoint(ESM::Pathgrid::Point point) const
        {
            toLocal(point);
            return point;
        }

        /// in-place conversion from local to world
        void toWorld(osg::Vec3f& point) const
        {
            point.x() += static_cast<float>(mCellX);
            point.y() += static_cast<float>(mCellY);
        }

        /// in-place conversion from world to local
        void toLocal(osg::Vec3f& point) const
        {
            point.x() -= static_cast<float>(mCellX);
            point.y() -= static_cast<float>(mCellY);
        }

        osg::Vec3f toLocalVec3(const osg::Vec3f& point) const
        {
            return osg::Vec3f(
                point.x() - static_cast<float>(mCellX), point.y() - static_cast<float>(mCellY), point.z());
        }

    private:
        int mCellX;
        int mCellY;
    };
}

#endif
