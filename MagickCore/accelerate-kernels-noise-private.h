/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private text included in place by accelerate-kernels-private.h,
  inside its accelerateKernels initialiser: the AddNoise kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d d N o i s e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  /*
  Part of MWC64X by David Thomas, dt10@imperial.ac.uk
  This is provided under BSD, full license is with the main package.
  See http://www.doc.ic.ac.uk/~dt10/research
  */

  // Pre: a<M, b<M
  // Post: r=(a+b) mod M
  ulong MWC_AddMod64(ulong a, ulong b, ulong M)
  {
    ulong v=a+b;
    //if( (v>=M) || (v<a) )
    if( (v>=M) || (convert_float(v) < convert_float(a)) ) // workaround for what appears to be an optimizer bug.
      v=v-M;
    return v;
  }

  // Pre: a<M,b<M
  // Post: r=(a*b) mod M
  // This could be done more efficiently, but it is portable, and should
  // be easy to understand. It can be replaced with any of the better
  // modular multiplication algorithms (for example if you know you have
  // double precision available or something).
  ulong MWC_MulMod64(ulong a, ulong b, ulong M)
  {
    ulong r=0;
    while(a!=0){
      if(a&1)
        r=MWC_AddMod64(r,b,M);
      b=MWC_AddMod64(b,b,M);
      a=a>>1;
    }
    return r;
  }

  // Pre: a<M, e>=0
  // Post: r=(a^b) mod M
  // This takes at most ~64^2 modular additions, so probably about 2^15 or so instructions on
  // most architectures
  ulong MWC_PowMod64(ulong a, ulong e, ulong M)
  {
    ulong sqr=a, acc=1;
    while(e!=0){
      if(e&1)
        acc=MWC_MulMod64(acc,sqr,M);
        sqr=MWC_MulMod64(sqr,sqr,M);
      e=e>>1;
    }
    return acc;
  }

  uint2 MWC_SkipImpl_Mod64(uint2 curr, ulong A, ulong M, ulong distance)
  {
    ulong m=MWC_PowMod64(A, distance, M);
    ulong x=curr.x*(ulong)A+curr.y;
    x=MWC_MulMod64(x, m, M);
    return (uint2)((uint)(x/A), (uint)(x%A));
  }

  uint2 MWC_SeedImpl_Mod64(ulong A, ulong M, uint vecSize, uint vecOffset, ulong streamBase, ulong streamGap)
  {
    // This is an arbitrary constant for starting LCG jumping from. I didn't
    // want to start from 1, as then you end up with the two or three first values
    // being a bit poor in ones - once you've decided that, one constant is as
    // good as any another. There is no deep mathematical reason for it, I just
    // generated a random number.
    enum{ MWC_BASEID = 4077358422479273989UL };

    ulong dist=streamBase + (get_global_id(0)*vecSize+vecOffset)*streamGap;
    ulong m=MWC_PowMod64(A, dist, M);

    ulong x=MWC_MulMod64(MWC_BASEID, m, M);
    return (uint2)((uint)(x/A), (uint)(x%A));
  }

  //! Represents the state of a particular generator
  typedef struct{ uint x; uint c; uint seed0; ulong seed1; } mwc64x_state_t;

  void MWC64X_Step(mwc64x_state_t *s)
  {
    uint X=s->x, C=s->c;

    uint Xn=s->seed0*X+C;
    uint carry=(uint)(Xn<C); // The (Xn<C) will be zero or one for scalar
    uint Cn=mad_hi(s->seed0,X,carry);

    s->x=Xn;
    s->c=Cn;
  }

  void MWC64X_Skip(mwc64x_state_t *s, ulong distance)
  {
    uint2 tmp=MWC_SkipImpl_Mod64((uint2)(s->x,s->c), s->seed0, s->seed1, distance);
    s->x=tmp.x;
    s->c=tmp.y;
  }

  void MWC64X_SeedStreams(mwc64x_state_t *s, ulong baseOffset, ulong perStreamOffset)
  {
    uint2 tmp=MWC_SeedImpl_Mod64(s->seed0, s->seed1, 1, 0, baseOffset, perStreamOffset);
    s->x=tmp.x;
    s->c=tmp.y;
  }

  //! Return a 32-bit integer in the range [0..2^32)
  uint MWC64X_NextUint(mwc64x_state_t *s)
  {
    uint res=s->x ^ s->c;
    MWC64X_Step(s);
    return res;
  }

  //
  // End of MWC64X excerpt
  //

  float mwcReadPseudoRandomValue(mwc64x_state_t* rng)
  {
    return (1.0f * MWC64X_NextUint(rng)) / (float)(0xffffffff); // normalized to 1.0
  }

  float mwcGenerateDifferentialNoise(mwc64x_state_t* r, float pixel, NoiseType noise_type, float attenuate)
  {
    float
      alpha,
      beta,
      noise,
      sigma;

    noise = 0.0f;
    alpha=mwcReadPseudoRandomValue(r);
    switch(noise_type)
    {
      case UniformNoise:
      default:
        {
          noise=(pixel+QuantumRange*SigmaUniform*(alpha-0.5f));
          break;
        }
      case GaussianNoise:
        {
          float
            gamma,
            tau;

          if (alpha == 0.0f)
            alpha=1.0f;
          beta=mwcReadPseudoRandomValue(r);
          gamma=sqrt(-2.0f*log(alpha));
          sigma=gamma*cospi((2.0f*beta));
          tau=gamma*sinpi((2.0f*beta));
          noise=pixel+sqrt(pixel)*SigmaGaussian*sigma+QuantumRange*TauGaussian*tau;
          break;
        }
      case ImpulseNoise:
      {
        if (alpha < (SigmaImpulse/2.0f))
          noise=0.0f;
        else
          if (alpha >= (1.0f-(SigmaImpulse/2.0f)))
            noise=QuantumRange;
          else
            noise=pixel;
        break;
      }
      case LaplacianNoise:
      {
        if (alpha <= 0.5f)
          {
            if (alpha <= MagickEpsilon)
              noise=(pixel-QuantumRange);
            else
              noise=(pixel+QuantumRange*SigmaLaplacian*log(2.0f*alpha)+
                0.5f);
            break;
          }
        beta=1.0f-alpha;
        if (beta <= (0.5f*MagickEpsilon))
          noise=(pixel+QuantumRange);
        else
          noise=(pixel-QuantumRange*SigmaLaplacian*log(2.0f*beta)+0.5f);
        break;
      }
      case MultiplicativeGaussianNoise:
      {
        sigma=1.0f;
        if (alpha > MagickEpsilon)
          sigma=sqrt(-2.0f*log(alpha));
        beta=mwcReadPseudoRandomValue(r);
        noise=(pixel+pixel*SigmaMultiplicativeGaussian*sigma*
          cospi((2.0f*beta))/2.0f);
        break;
      }
      case PoissonNoise:
      {
        float
          poisson;
        unsigned int i;
        poisson=exp(-SigmaPoisson*QuantumScale*pixel);
        for (i=0; alpha > poisson; i++)
        {
          beta=mwcReadPseudoRandomValue(r);
          alpha*=beta;
        }
        noise=(QuantumRange*i*MagickSafeReciprocal(SigmaPoisson));
        break;
      }
      case RandomNoise:
      {
        noise=(QuantumRange*SigmaRandom*alpha);
        break;
      }
    }
    return noise;
  }
  )

