
# The idea of plugins and the `SimulatorAccess` and `Introspection` classes

The most common modification you will probably want to do to
ASPECT are to switch to a different material model
(i.e., have different values of functional dependencies for the coefficients
$\eta,\rho,C_p, \ldots$ discussed in {ref}`sec:coefficients`8]);
change the geometry; change the direction and magnitude of the gravity vector
$\mathbf g$; or change the initial and boundary conditions.

To make this as simple as possible, all of these parts of the program (and
some more) have been separated into what we call *plugins* that can be
replaced quickly and where it is simple to add a new implementation and make
it available to the rest of the program and the input parameter file. There
are *a lot* of plugins already, see Fig.&nbsp;[1][], that will often be useful
starting points and examples if you want to implement plugins yourself.

<figure>
<embed src="plugin_graph.pdf" id="fig:plugins" style="width:95.0%" /><figcaption aria-hidden="true"><em>The graph of all current plugins of ASPECT. The yellow octagon and square represent the <code>Simulator</code> and <code>SimulatorAccess</code> classes. The green boxes are interface classes for everything that can be changed by plugins. Blue circles correspond to plugins that implement particular behavior. The graph is of course too large to allow reading individual plugin names (unless you zoom far into the page), but is intended to illustrate the architecture of ASPECT.</em></figcaption>
</figure>

The way this is achieved is through the following two steps:

-   The core of ASPECT really only communicates
    with material models, geometry descriptions, etc., through a simple and
    very basic interface. These interfaces are declared in the
    [include/aspect/material_model/interface.h][],
    [include/aspect/geometry_model/interface.h][], etc., header files. These
    classes are always called `Interface`, are located in namespaces that
    identify their purpose, and their documentation can be found from the
    general class overview in
    <https://aspect.geodynamics.org/doc/doxygen/classes.html>.

    To show an example of a rather minimal case, here is the declaration of
    the [aspect::GravityModel::Interface][] class (documentation comments have
    been removed):

    ```{code-block} c++
    class Interface
        {
          public:
            virtual ~Interface();

            virtual
            Tensor<1,dim>
            gravity_vector (const Point<dim> &position) const = 0;

            static void declare_parameters (ParameterHandler &prm);

            virtual void parse_parameters (ParameterHandler &prm);
        };
    ```

    If you want to implement a new model for gravity, you just need to write a
    class that derives from this base class and implements the
    `gravity_vector` function. If your model wants to read parameters from the
    input file, you also need to have functions called `declare_parameters`
    and `parse_parameters` in your class with the same signatures as the ones
    above. On the other hand, if the new model does not need any run-time
    parameters, you do not need to overload these functions.[1]

    Each of the categories above that allow plugins have several
    implementations of their respective interfaces that you can use to get an
    idea of how to implement a new model.

-   At the end of the file where you implement your new model, you need to
    have a call to the macro `ASPECT_REGISTER_GRAVITY_MODEL` (or the
    equivalent for the other kinds of plugins). For example, let us say that
    you had implemented a gravity model that takes actual gravimetric readings
    from the GRACE satellites into account, and had put everything that is
    necessary into a class `aspect::GravityModel::GRACE`. Then you need a
    statement like this at the bottom of the file:

    ```{code-block} c++
    ASPECT_REGISTER_GRAVITY_MODEL
        (GRACE,
         "grace",
         "A gravity model derived from GRACE "
         "data. Run-time parameters are read from the parameter "
         "file in subsection 'Radial constant'.");
    ```

    Here, the first argument to the macro is the name of the class. The second
    is the name by which this model can be selected in the parameter file. And
    the third one is a documentation string that describes the purpose of the
    class (see, for example, {ref}`sec:3.55][] for an example of how
    existing models describe themselves).

    This little piece of code ensures several things: (i) That the parameters
    this class declares are known when reading the parameter file. (ii) That
    you can select this model (by the name "grace") via the
    run-time parameter `Gravity model/Model name`. (iii) That
    ASPECT can create an object of this kind when
    selected in the parameter file.

    Note that you need not announce the existence of this class in any other
    part of the code: Everything should just work automatically.[2] This has
    the advantage that things are neatly separated: You do not need to
    understand the core of ASPECT to be able to
    add a new gravity model that can then be selected in an input file. In
    fact, this is true for all of the plugins we have: by and large, they just
    receive some data from the simulator and do something with it (e.g.,
    postprocessors), or they just provide information (e.g., initial meshes,
    gravity models), but their writing does not require that you have a
    fundamental understanding of what the core of the program does.

The procedure for the other areas where plugins are supported works
essentially the same, with the obvious change in namespace for the interface
class and macro name.

In the following, we will discuss the requirements for individual plugins.
Before doing so, however, let us discuss ways in which plugins can query other
information, in particular about the current state of the simulation. To this
end, let us not consider those plugins that by and large just provide
information without any context of the simulation, such as gravity models,
prescribed boundary velocities, or initial temperatures. Rather, let us
consider things like postprocessors that can compute things like boundary heat
fluxes. Taking this as an example (see {ref}`sec:1.4.8][]), you are
required to write a function with the following interface

```{code-block} c++
template <int dim>
    class MyPostprocessor : public aspect::Postprocess::Interface
    {
      public:
        virtual
        std::pair<std::string,std::string>
        execute (TableHandler &statistics);

      // ... more things ...
```

The idea is that in the implementation of the `execute` function you would
compute whatever you are interested in (e.g., heat fluxes) and return this
information in the statistics object that then gets written to a file (see
Sections&nbsp;[\[sec:running-overview`9] and [\[sec:viz-stat`10]). A
postprocessor may also generate other files if it so likes &ndash; e.g.,
graphical output, a file that stores the locations of particles, etc. To do
so, obviously you need access to the current solution. This is stored in a
vector somewhere in the core of ASPECT.
However, this vector is, by itself, not sufficient: you also need to know the
finite element space it is associated with, and for that the triangulation it
is defined on. Furthermore, you may need to know what the current simulation
time is. A variety of other pieces of information enters computations in these
kinds of plugins.

All of this information is of course part of the core of
ASPECT, as part of the [aspect::Simulator class][].
However, this is a rather heavy class: it's got dozens of member
variables and functions, and it is the one that does all of the numerical
heavy lifting. Furthermore, to access data in this class would require that
you need to learn about the internals, the data structures, and the design of
this class. It would be poor design if plugins had to access information from
this core class directly. Rather, the way this works is that those plugin
classes that wish to access information about the state of the simulation
inherit from the [aspect::SimulatorAccess class][]. This class has an
interface that looks like this:

```{code-block} c++
template <int dim>
    class SimulatorAccess
    {
    protected:
      double       get_time () const;

      std::string  get_output_directory () const;

      const LinearAlgebra::BlockVector &
      get_solution () const;

      const DoFHandler<dim> &
      get_dof_handler () const;

      // ... many more things ...
```

This way, [SimulatorAccess][aspect::SimulatorAccess class] makes information
available to plugins without the need for them to understand details of the
core of ASPECT. Rather, if the core changes,
the [SimulatorAccess][aspect::SimulatorAccess class] class can still provide
exactly the same interface. Thus, it insulates plugins from having to know the
core. Equally importantly, since
[SimulatorAccess][aspect::SimulatorAccess class] only offers its information
in a read-only way it insulates the core from plugins since they can not
interfere in the workings of the core except through the interface they
themselves provide to the core.

Using this class, if a plugin class `MyPostprocess` is then not only derived
from the corresponding `Interface` class but *also* from the
[SimulatorAccess][aspect::SimulatorAccess class] class (as indeed most plugins
are, see the dashed arrows in Fig.&nbsp;[1][]), then you can write a member
function of the following kind (a nonsensical but instructive example; see
{ref}`sec:1.4.8][] for more details on what postprocessors do and how they
are implemented):[3]

```{code-block} c++
template <int dim>
    std::pair<std::string,std::string>
    MyPostprocessor<dim>::execute (TableHandler &statistics)
    {
      // compute the mean value of vector component 'dim' of the solution
      // (which here is the pressure block) using a deal.II function:
      const double
        average_pressure = VectorTools::compute_mean_value (this->get_mapping(),
                                                            this->get_dof_handler(),
                                                            QGauss<dim>(2),
                                                            this->get_solution(),
                                                            dim);
      statistics.add_value ("Average pressure", average_pressure);

      // return that there is nothing to print to screen (a useful
      // plugin would produce something more elaborate here):
      return std::pair<std::string,std::string>();
    }
```

The second piece of information that plugins can use is called
"introspection." In the code snippet above, we had to use that the
pressure variable is at position `dim`. This kind of *implicit knowledge* is
usually bad style: it is error prone because one can easily forget where each
component is located; and it is an obstacle to the extensibility of a code if
this kind of knowledge is scattered all across the code base.

Introspection is a way out of this dilemma. Using the
`SimulatorAccess::introspection()` function returns a reference to an object
(of type [aspect::Introspection][]) that plugins can use to learn about these
sort of conventions. For example,
`this->introspection().component_mask.pressure` returns a component mask (a
deal.II concept that describes a list of booleans for each component in a
finite element that are true if a component is part of a variable we would
like to select and false otherwise) that describes which component of the
finite element corresponds to the pressure. The variable, `dim`, we need above
to indicate that we want the pressure component can be accessed as
`this->introspection().component_indices.pressure`. While this is certainly
not shorter than just writing `dim`, it may in fact be easier to remember. It
is most definitely less prone to errors and makes it simpler to extend the
code in the future because we don't litter the sources with "magic
constants" like the one above.

This [aspect::Introspection][] class has a significant number of variables
that can be used in this way, i.e., they provide symbolic names for things one
frequently has to do and that would otherwise require implicit knowledge of
things such as the order of variables, etc.
