#include <graphlab.hpp>
#include <iostream>

template <class T>
class vector2D {
  public:
    T x,y;

  public:
    vector2D()
      {}

    vector2D(T nx,T ny)
      {x=nx; y=ny;}

    vector2D<T> &operator=(vector2D<T> p)
      {set(p.x, p.y); return(*this);}

    /// set the components of the vector
    void set(T nx,T ny)
      {x=nx; y=ny;}

    /// zero all components of the vector void zero()
    void zero()
      {x=y=0;}
};

typedef vector2D<float> vector2f;

class Particle2D {
  public:
    vector2f loc;
    float angle;
    float weight;

  public:
    Particle2D() {weight = angle = 0.0; loc.zero();}

    Particle2D(float _x, float _y, float _theta, float _w) { loc.set(_x,_y); angle = _theta; weight = _w;}

    Particle2D(vector2f _loc, float _theta, float _w) { loc = _loc; angle = _theta; weight = _w;}

    bool operator<(const Particle2D &other) {return weight<other.weight;}

    bool operator>(const Particle2D &other) {return weight>other.weight;}

    Particle2D &operator=(const Particle2D &other) {
      angle = other.angle;
      weight = other.weight;
      loc.x = other.loc.x;
      loc.y = other.loc.y;
      return *this;
    }

    void save(graphlab::oarchive& oarc) const {
      oarc << angle << weight << loc.x << loc.y;
    }

    void load(graphlab::iarchive& iarc) {
      iarc >> angle >> weight >> loc.x >> loc.y;
    }
};

typedef graphlab::distributed_graph<Particle2D, graphlab::empty> graph_type;

void RandomParticle(graph_type::vertex_type& v) {
  v.data().loc.set(graphlab::random::gaussian(), graphlab::random::gaussian());
  v.data().angle = graphlab::random::gaussian();
}

int main(int argc, char** argv) {
  graphlab::mpi_tools::init(argc, argv);
  graphlab::distributed_control dc;

  graph_type graph(dc);

  const unsigned long long NUM_PARTICLES = 1e3;
  // populate the graph with vertices
  for (unsigned long long int i = 0; i < NUM_PARTICLES; i++)
    graph.add_vertex(i, Particle2D());

  // commit the graph structure, marking that it is no longer to be modified
  graph.finalize();

  dc.cout() << "num_local_vertices = "  << graph.num_local_vertices() << std::endl;
  dc.cout() << "num_local_own_vertices = "  << graph.num_local_own_vertices() << std::endl;
  dc.cout() << "num_local_edges = "  << graph.num_local_edges() << std::endl;

  int NUM_ITERATIONS;
  if (argc > 1) NUM_ITERATIONS = atoi(argv[1]);
  else          NUM_ITERATIONS = 100;

  graphlab::timer timer;
  timer.start();
  for (int i = 0; i < NUM_ITERATIONS; i++) graph.transform_vertices(RandomParticle);
  dc.cout() << "Elapsed time: " << timer.current_time() << std::endl;

  graphlab::mpi_tools::finalize();

  return 0;
}
