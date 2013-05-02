Name: moses
Summary: Moses is a statistical machine translation system that allows you to automatically train translation models for any language pair.
Version: ___RPM_VERSION__
Release: 1
URL: http://www.statmt.org/moses/
Source0: %{name}-%{version}.tar.gz
License: LGPL
Group: Development/Tools
Vendor: Capita Translation and Interpreting
Packager: Ian Johnson <ian.johnson@capita-ti.com>
Requires: boost >= 1.48, python >= 2.6, perl >= 5
BuildRoot: /home/ian/rpmbuild/builds/%{name}-%{version}-%{release}
%description
Moses is a statistical machine translation system that allows you to automatically train translation models for any language pair. All you need is a collection of translated texts (parallel corpus). An efficient search algorithm finds quickly the highest probability translation among the exponential number of choices.
%prep
%setup -q

mkdir -p $RPM_BUILD_ROOT/opt/moses/giza++-v1.0.7

wget -O $RPM_BUILD_DIR/irstlm-5.70.04.tgz http://moses-suite.googlecode.com/files/irstlm-5.70.04.tgz 
wget -O $RPM_BUILD_DIR/giza-pp-v1.0.7.tgz http://moses-suite.googlecode.com/files/giza-pp-v1.0.7.tar.gz

cd $RPM_BUILD_DIR

tar -zxf irstlm-5.70.04.tgz
tar -zxf giza-pp-v1.0.7.tgz

cd irstlm-5.70.04
bash regenerate-makefiles.sh --force
./configure --prefix $RPM_BUILD_ROOT/opt/moses/irstlm-5.70.04
make
make install

cd ../giza-pp
make
cp $RPM_BUILD_DIR/giza-pp/GIZA++-v2/GIZA++ $RPM_BUILD_DIR/giza-pp/GIZA++-v2/snt2cooc.out $RPM_BUILD_DIR/giza-pp/mkcls-v2/mkcls $RPM_BUILD_ROOT/opt/moses/giza++-v1.0.7
%build
./bjam --with-irstlm=$RPM_BUILD_ROOT/opt/moses/irstlm-5.70.04 --with-giza=$RPM_BUILD_ROOT/opt/moses/giza++-v1.0.7 -j2
%install
mkdir -p $RPM_BUILD_ROOT/opt/moses/scripts
cp -R bin $RPM_BUILD_ROOT/opt/moses
cp -R scripts/analysis $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/ems $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/generic $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/other $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/recaser $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/regression-testing $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/share $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/tokenizer $RPM_BUILD_ROOT/opt/moses/scripts
cp -R scripts/training $RPM_BUILD_ROOT/opt/moses/scripts
%clean
%files
%defattr(-,root,root)
/opt/moses/bin/*
/opt/moses/scripts/analysis/*
/opt/moses/scripts/ems/*
/opt/moses/scripts/generic/*
/opt/moses/scripts/other/*
/opt/moses/scripts/recaser/*
/opt/moses/scripts/regression-testing/*
/opt/moses/scripts/share/*
/opt/moses/scripts/tokenizer/*
/opt/moses/scripts/training/*
/opt/moses/irstlm-5.70.04/*
/opt/moses/giza++-v1.0.7/*
