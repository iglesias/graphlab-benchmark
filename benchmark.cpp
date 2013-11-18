#include <graphlab.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

template <class T> class vector2D;
class Particle2D;
class Resampler;

typedef Resampler gather_type;
typedef vector2D<float> vector2f;
typedef graphlab::distributed_graph<Particle2D, graphlab::empty> graph_type;

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
      counter = 0;
    }

    Particle2D(float _x, float _y, float _theta, float _w) {
      loc.set(_x,_y);
      angle = _theta;
      weight = _w;
      counter = 0;
    }

    Particle2D(vector2f _loc, float _theta, float _w) {
      loc = _loc;
      angle = _theta;
      weight = _w;
      counter = 0;
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
    int counter;
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

      ++vertex.data().counter;
      if (vertex.data().counter < 10) context.signal(vertex);
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

void Map(int num_particles, int num_iterations) {
  std::cout << "Map\n";
  graphlab::distributed_control dc;
  graph_type graph(dc);

  // populate the graph with vertices
  for (int i = 0; i < num_particles; i++)
    graph.add_vertex(i, Particle2D());

  // commit the graph structure, marking that it is no longer to be modified
  graph.finalize();

  dc.cout() << "num_local_vertices = "  << graph.num_local_vertices() << std::endl;
  dc.cout() << "num_local_own_vertices = "  << graph.num_local_own_vertices() << std::endl;
  dc.cout() << "num_local_edges = "  << graph.num_local_edges() << std::endl;

  graphlab::timer timer;
  timer.start();
  for (int i = 0; i < num_iterations; i++) graph.transform_vertices(RandomParticle);
  dc.cout() << "Elapsed time: " << timer.current_time() << std::endl;
}

void Resample(int num_particles, int num_iterations, bool sparse) {
  std::cout << "Resample\n";

  graphlab::distributed_control dc;
  graph_type graph(dc);

  // populate the graph with vertices and eges
  if (sparse) {
    for (int i = 0; i < num_particles-1; i++) {
      graph.add_edge(i, i+1);
      graph.add_edge(i+1, i);
    }
  } else {
    for (int i = 0; i < num_particles; i++) {
      for (int j = i+1; j < num_particles; j++) {
        graph.add_edge(i, j);
        graph.add_edge(j, i);
      }
    }
  }

  // commit the graph structure, marking that it is no longer to be modified
  graph.finalize();

  dc.cout() << "num_local_vertices = "  << graph.num_local_vertices() << std::endl;
  dc.cout() << "num_local_own_vertices = "  << graph.num_local_own_vertices() << std::endl;
  dc.cout() << "num_local_edges = "  << graph.num_local_edges() << std::endl;

  graphlab::omni_engine<ResamplerProgram> engine(dc, graph, "sync");

  graphlab::timer timer;
  timer.start();

  engine.signal_all();
  engine.start();

  dc.cout() << "Elapsed time: " << timer.current_time() << std::endl;
}

int main(int argc, char** argv) {
  graphlab::command_line_options clopts("Particle filter");

  int num_particles = 100;
  std::string mode = "resample";
  int num_iterations = 10;
  bool sparse = true;
  clopts.attach_option("particles", num_particles, "Number of particles.");
  clopts.attach_option("mode", mode, "Execution mode. Vertex transformation (map) or vertex program (resample).");
  clopts.attach_option("iterations", num_iterations, "Number of repetitions.");
  clopts.attach_option("sparse", sparse, "For resample mode, use sparse graph representation.");

  if (!clopts.parse(argc, argv))  return EXIT_FAILURE;

  if (num_iterations <= 0) {
    std::cerr << "The number of iterations must be larger than zero\n";
    return EXIT_FAILURE;
  }

  if (num_particles <= 0) {
    std::cerr << "The number of particles must be larger than zero\n";
    return EXIT_FAILURE;
  }

  global_logger().set_log_level(LOG_NONE);
  graphlab::mpi_tools::init(argc, argv);

  if (mode == "map")            Map(num_particles, num_iterations);
  else if (mode == "resample")  Resample(num_particles, num_iterations, sparse);
  else                          std::cerr << "Unknown mode given " << mode << ". The options are map or resample.\n";

  graphlab::mpi_tools::finalize();

  return 0;
}
