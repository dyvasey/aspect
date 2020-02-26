/*
  Copyright (C) 2018 by the authors of the World Builder code.

  This file is part of the World Builder.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _world_builder_features_interface_h
#define _world_builder_features_interface_h

#include <map>
#include <vector>

#include <world_builder/world.h>
#include <world_builder/parameters.h>
#include <world_builder/point.h>

using namespace std;

namespace WorldBuilder
{
  class World;


  namespace Features
  {
    class ObjectFactory;

    /**
     * This class is an interface for the specific plate tectonic feature classes,
     * such as continental plate, oceanic plate and subduction zone.
     */
    class Interface
    {
      public:
        /**
         * constructor
         */
        Interface();

        /**
         * Destructor
         */
        virtual
        ~Interface();


        /**
         * depricated
         */
        void
        declare_interface_entries(Parameters &prm,
                                  const CoordinateSystem coordinate_system);
        /**
         * helper function to parse coordinates.
         */
        void
        get_coordinates(const std::string name,
                        Parameters &prm,
                        const CoordinateSystem coordinate_system);

        /**
         * declare and read in the world builder file into the parameters class
         */
        static
        void declare_entries(Parameters &prm,
                             const std::string &parent_name,
                             const std::vector<std::string> &required_entries);

        /**
         * declare and read in the world builder file into the parameters class
         */
        virtual
        void parse_entries(Parameters &prm) = 0;


        /**
         * takes temperature and position and returns a temperature.
         */
        virtual
        double temperature(const Point<3> &position,
                           const double depth,
                           const double gravity,
                           double temperature) const = 0;
        /**
         * Returns a value for the requested composition (0 is not present,
         * 1 is present) based on the given position and
         */
        virtual
        double composition(const Point<3> &position,
                           const double depth,
                           const unsigned int composition_number,
                           double value) const = 0;


        /**
         * A function to register a new type. This is part of the automatic
         * registration of the object factory.
         */
        static void registerType(const std::string &name,
                                 void ( *)(Parameters &, const std::string &, const std::vector<std::string> &required_entries),
                                 ObjectFactory *factory);

        const std::string get_name() const
        {
          return name;
        };


        /**
         * A function to create a new type. This is part of the automatic
         * registration of the object factory.
         */
        static std::unique_ptr<Interface> create(const std::string &name, WorldBuilder::World *world);

      protected:
        /**
         * A pointer to the world class to retrieve variables.
         */
        WorldBuilder::World *world;

        /**
         * The name of the feature type.
         */
        std::string name;

        /**
         * number of original coordinates, before adding
         * more automatically.
         */
        unsigned int original_number_of_coordinates;

        /**
         * The coordinates at the surface of the feature
         */
        std::vector<Point<2> > coordinates;

        /**
         * A vector of one dimensional coordinates for this feature.
         * If empty, this variables is interpretated just as
         * {0,1,2,...,number of coordinates}. It allows for, for example,
         * adding extra coordinates automatically, and still reference the
         * user provided coordinates by the original number. Note that no
         * whole numbers may be skiped. So for a list of 4 points, {0,0.5,1,2}
         * is allowed, but {0,2,3,4} is not.
         */
        std::vector<double> one_dimensional_coordinates;


        /**
         * The name of the temperature submodule used by this feature.
         */
        std::string temperature_submodule_name;

        /**
         * The name of the composition submodule used by this feature.
         */
        std::string composition_submodule_name;


      private:
        static std::map<std::string, ObjectFactory *> &get_factory_map()
        {
          static std::map<std::string, ObjectFactory *> factories;
          return factories;
        }

        static std::map<std::string, void ( *)(Parameters &,
                                               const std::string &,
                                               const std::vector<std::string>& required_entries)> &get_declare_map()
        {
          static std::map<std::string, void ( *)(Parameters &,
                                                 const std::string &,
                                                 const std::vector<std::string>& required_entries)> declares;
          return declares;
        }
    };


    /**
     * A class to create new objects
     */
    class ObjectFactory
    {
      public:
        virtual std::unique_ptr<Interface> create(World *world) = 0;
    };

    /**
     * A macro which should be in every derived cpp file to automatically
     * register it. Because this is a library, we need some extra measures
     * to ensure that the static variable is actually initialized.
     */
#define WB_REGISTER_FEATURE(klass,name) \
  class klass##Factory : public ObjectFactory { \
    public: \
      klass##Factory() \
      { \
        Interface::registerType(#name, klass::declare_entries, this); \
      } \
      virtual std::unique_ptr<Interface> create(World *world) { \
        return std::unique_ptr<Interface>(new klass(world)); \
      } \
  }; \
  static klass##Factory global_##klass##Factory;

  }
}

#endif