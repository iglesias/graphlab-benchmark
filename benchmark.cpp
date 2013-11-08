#include <graphlab.hpp>
#include <iostream>
#include <vector>
#include <cassert>

template <class T> class vector2D;
class Particle2D;
class Resampler;

typedef graphlab::distributed_graph<Particle2D, graphlab::empty> graph_type;
typedef Resampler gather_type;
typedef vector2D<float> vector2f;

template <class T>
class vector2D {
  public:
    vector2D() { }

    vector2D(T nx,T ny) {
      x = nx;
      y = ny;
    }

    vector2D<T> &operator=(const vector2D<T>& p) {
      set(p.x, p.y);
      return *this;
    }

    /// set the components of the vector
    void set(T nx,T ny) {
      x = nx;
      y = ny;
    }

    /// zero all components of the vector void zero()
    void zero() {
      x = y = 0;
    }

  public:
    T x,y;
};

class Particle2D {
  public:
    Particle2D() {
      weight = angle = 0.0;
      loc.zero();
    }

    Particle2D(float _x, float _y, float _theta, float _w) {
      loc.set(_x,_y);
      angle = _theta;
      weight = _w;
    }

    Particle2D(vector2f _loc, float _theta, float _w) {
      loc = _loc;
      angle = _theta;
      weight = _w;
    }

    bool operator<(const Particle2D &other) {
      return weight<other.weight;
    }

    bool operator>(const Particle2D &other) {
      return weight>other.weight;
    }

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

  public:
    vector2f loc;
    float angle;
    float weight;
};

class Resampler {
  public:
    Resampler() { };

    explicit Resampler(const Particle2D& particle) {
      in_particles.push_back(particle);
    }

    Resampler& operator+=(const Resampler& other) {
      for (unsigned int i = 0; i < other.in_particles.size(); i++)
        in_particles.push_back(other.in_particles[i]);

      return *this;
    }

    void save(graphlab::oarchive& oarc) const {
      oarc << in_particles;
    }

    void load(graphlab::iarchive& iarc) {
      iarc >> in_particles;
    }

  public:
    std::vector<Particle2D> in_particles;
};

class ResamplerProgram : public graphlab::ivertex_program<graph_type, gather_type>, public graphlab::IS_POD_TYPE {
  public:
    edge_dir_type gather_edges(icontext_type& context, const vertex_type& vertex) const {
      return graphlab::IN_EDGES;
    }

    gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const {
      return Resampler(edge.source().data());
    }

    void apply(icontext_type& context, vertex_type& vertex, const gather_type& total) {
      // compute the CDF of the particles that participate in this resampling
      std::vector<float> weight_cdf(1+total.in_particles.size(), 0.0);
      weight_cdf[0] = vertex.data().weight;
      for (unsigned int i = 0; i < total.in_particles.size(); ++i)
        weight_cdf[i+1] = weight_cdf[i] + total.in_particles[i].weight;

      // resample
      float beta = graphlab::random::uniform(0.0f, weight_cdf[weight_cdf.size()-1]);
      unsigned int i = 0;
      while (weight_cdf[i] < beta) i++;
      if (i > 0) {
        assert(i <= total.in_particles.size());
        // i-1 because the the first weight in weight_cdf corresponds to the current vertex
        vertex.data() = total.in_particles[i-1];
      } else {
        assert(i == 0);
      }
    }

    edge_dir_type scatter_edges(icontext_type& context, const vertex_type& vertex) const {
      return graphlab::NO_EDGES;
    }

    void scatter(icontext_type& context, const vertex_type& vertex, const edge_type& edge) const { }
};

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
