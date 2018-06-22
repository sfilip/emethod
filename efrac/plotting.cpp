#include "plotting.h"
#include <cstdlib>
#include <fstream>

void plotFunc(std::string &filename,
              std::function<mpfr::mpreal(mpfr::mpreal)> &f, mpfr::mpreal &a,
              mpfr::mpreal &b, mp_prec_t prec)
{
  using mpfr::mpreal;
  mp_prec_t prevPrec = mpreal::get_default_prec();
  mpreal::set_default_prec(prec);

  std::stringstream datFilename;
  datFilename << filename << "_0.dat";

  std::ofstream output;
  output.open(datFilename.str().c_str());
  mpfr::mpreal width = b - a;
  mpfr::mpreal buffer, bufferValue;
  std::size_t pointCount = 5000u;
  mpfr::mpreal maxValue = 0u;
  for (std::size_t i = 0u; i < pointCount; ++i)
  {
    buffer = a + (width * i) / pointCount;
    output << buffer.toString("%.80RNf") << "\t";
    bufferValue = f(buffer);
    if (mpfr::abs(bufferValue) > maxValue)
      maxValue = mpfr::abs(bufferValue);
    output << bufferValue.toString("%.80RNf") << std::endl;
  }
  output << b.toString("%.80RNf") << "\t";
  bufferValue = f(b);
  output << bufferValue.toString("%.80RNf") << std::endl;
  if (mpfr::abs(bufferValue) > maxValue)
    maxValue = mpfr::abs(bufferValue);
  std::cout << "Max value = " << maxValue << std::endl;
  output.close();

  std::stringstream gnuplotFile;
  gnuplotFile << filename << "_f.p";
  std::stringstream epsFilename;
  epsFilename << filename << "_f.eps";
  output.open(gnuplotFile.str().c_str());

  output << "set terminal postscript eps color\n"
         << R"(set out ")" << epsFilename.str() << R"(")" << std::endl
         << R"(set format x "%g")" << std::endl
         << R"(set format y "%g")" << std::endl
         << "set xrange [" << a.toString("%.80RNf") << ":"
         << b.toString("%.80RNf") << "]\n"
         << R"(plot ")" << datFilename.str() << R"(" using 1:2 with lines t "")"
         << std::endl;

  std::string gnuplotCommand = "gnuplot ";
  gnuplotCommand += gnuplotFile.str();
  system(gnuplotCommand.c_str());

  mpreal::set_default_prec(prevPrec);
}

void plotFuncEtVals(std::string &filename,
                    std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                    std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> &p,
                    mpfr::mpreal &a, mpfr::mpreal &b, mp_prec_t prec)
{
  using mpfr::mpreal;
  mp_prec_t prevPrec = mpreal::get_default_prec();
  mpreal::set_default_prec(prec);

  std::stringstream datFilename;
  datFilename << filename << "_0.dat";

  std::ofstream output;
  output.open(datFilename.str().c_str());
  mpfr::mpreal width = b - a;
  mpfr::mpreal buffer, bufferValue;
  std::size_t pointCount = 10000u;
  for (std::size_t i = 0u; i < pointCount; ++i)
  {
    buffer = a + (width * i) / pointCount;
    // output << buffer.toString("%.80RNf") << "\t";
    output << ((buffer + 1) / 2).toString("%.80RNf") << "\t";
    bufferValue = f(buffer);
    output << bufferValue.toString("%.80RNf") << std::endl;
  }
  //output << b.toString("%.80RNf") << "\t";
  output << ((b + 1) / 2).toString("%.80RNf") << "\t";
  bufferValue = f(b);
  output << bufferValue.toString("%.80RNf") << std::endl;

  output.close();

  std::stringstream pointFilename;
  pointFilename << filename << "_1.dat";

  std::ofstream output2;
  output2.open(pointFilename.str().c_str());
  for (std::size_t i{0u}; i < p.size(); ++i)
    /*output2 << p[i].first.toString("%.80RNf") << "\t"
            << p[i].second.toString("%.80RNf") << std::endl;*/
    output2 << ((p[i].first + 1) / 2).toString("%.80RNf") << "\t"
            << p[i].second.toString("%.80RNf") << std::endl;

  output2.close();

  std::stringstream gnuplotFile;
  gnuplotFile << filename << "_f.p";
  std::stringstream epsFilename;
  epsFilename << filename << "_f.eps";
  output.open(gnuplotFile.str().c_str());

  output << "set terminal postscript eps color\n"
         << R"(set out ")" << epsFilename.str() << R"(")" << std::endl
         << R"(set format x "%g")" << std::endl
         << R"(set format y "%g")" << std::endl
         << "set xrange [" << ((a + 1) / 2).toString("%.80RNf") << ":"
         << ((b + 1) / 2).toString("%.80RNf") << "]\n"
         << R"(plot ")" << datFilename.str()
         << R"(" using 1:2 with lines t "", \)" << std::endl
         << "\t"
         << R"(")" << pointFilename.str() << R"(" using 1:2 with points t "")"
         << std::endl;

  std::string gnuplotCommand = "gnuplot ";
  gnuplotCommand += gnuplotFile.str();
  system(gnuplotCommand.c_str());

  mpreal::set_default_prec(prevPrec);
}

void plotFuncs(std::string &filename,
               std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> &fs,
               mpfr::mpreal &a, mpfr::mpreal &b, mp_prec_t prec)
{
  using mpfr::mpreal;
  mp_prec_t prevPrec = mpreal::get_default_prec();
  mpreal::set_default_prec(prec);

  std::vector<std::stringstream> datFilename(fs.size());
  for (std::size_t i{0u}; i < fs.size(); ++i)
  {
    datFilename[i] << filename << "_" << (i + 1u) << ".dat";
    std::ofstream output;
    output.open(datFilename[i].str().c_str());
    mpfr::mpreal width = b - a;
    mpfr::mpreal buffer, bufferValue;
    std::size_t pointCount = 5000u;
    for (std::size_t j{0u}; j < pointCount; ++j)
    {
      buffer = a + (width * j) / pointCount;
      output << buffer.toString("%.80RNf") << "\t";
      bufferValue = fs[i](buffer);
      output << bufferValue.toString("%.80RNf") << std::endl;
    }
    output << b.toString("%.80RNf") << "\t";
    bufferValue = fs[i](b);
    output << bufferValue.toString("%.80RNf") << std::endl;

    output.close();
  }

  std::stringstream gnuplotFile;
  gnuplotFile << filename << "_f.p";
  std::stringstream epsFilename;
  epsFilename << filename << "_f.eps";
  std::ofstream output;
  output.open(gnuplotFile.str().c_str());

  output << "set terminal postscript eps color\n"
         << R"(set out ")" << epsFilename.str() << R"(")" << std::endl
         << R"(set format x "%g")" << std::endl
         << R"(set format y "%g")" << std::endl
         << "set xrange [" << a.toString("%.30RNf") << ":"
         << b.toString("%.30RNf") << "]\n"
         << R"(plot ")" << datFilename[0].str()
         << R"(" using 1:2 with lines t "", \)" << std::endl;

  for (std::size_t i{1u}; i < fs.size() - 1; ++i)
  {
    output << "\t"
           << R"(")" << datFilename[i].str()
           << R"(" using 1:2 with lines t "", \)" << std::endl;
  }
  if (fs.size() > 1u)
    output << "\t"
           << R"(")" << datFilename[fs.size() - 1].str()
           << R"(" using 1:2 with lines t "")" << std::endl;

  output.close();

  std::string gnuplotCommand = "gnuplot ";
  gnuplotCommand += gnuplotFile.str();
  system(gnuplotCommand.c_str());

  mpreal::set_default_prec(prevPrec);
}

void plotVals(std::string &filename,
              std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> &p,
              mpfr::mpreal &a, mpfr::mpreal &b, mp_prec_t prec)
{

  using mpfr::mpreal;
  mp_prec_t prevPrec = mpreal::get_default_prec();
  mpreal::set_default_prec(prec);

  std::stringstream datFilename;
  datFilename << filename << "_0.dat";

  std::ofstream output;
  output.open(datFilename.str().c_str());
  for (std::size_t i{0u}; i < p.size(); ++i)
    output << p[i].first.toString("%.80RNf") << "\t"
           << p[i].second.toString("%.80RNf") << std::endl;

  output.close();

  std::stringstream gnuplotFile;
  gnuplotFile << filename << "_f.p";
  std::stringstream epsFilename;
  epsFilename << filename << "_f.eps";
  output.open(gnuplotFile.str().c_str());

  output << "set terminal postscript eps color\n"
         << R"(set out ")" << epsFilename.str() << R"(")" << std::endl
         << R"(set format x "%g")" << std::endl
         << R"(set format y "%g")" << std::endl
         << "set xrange [" << a.toString("%.80RNf") << ":"
         << b.toString("%.80RNf") << "]\n"
         << R"(plot ")" << datFilename.str() << R"(" using 1:2 with lines t "")"
         << std::endl;

  std::string gnuplotCommand = "gnuplot ";
  gnuplotCommand += gnuplotFile.str();
  system(gnuplotCommand.c_str());

  mpreal::set_default_prec(prevPrec);
}
