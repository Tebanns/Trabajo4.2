-- ============================================================
-- PROYECTO IOT - EVALUACIÓN SUMATIVA 3 - DCSH01
-- Ejecutar este script en Supabase: Panel -> SQL Editor -> New query
-- ============================================================

create table if not exists eventos (
    id               bigint generated always as identity primary key,
    created_at       timestamptz not null default now(),
    pot_valor        integer,
    pot_porcentaje   numeric(5,1),
    movimiento       boolean not null default false,
    led_alerta       boolean not null default false,
    comando_manual   boolean not null default false
);

-- (Opcional) Habilitar acceso público de lectura/escritura para pruebas.
-- En un entorno real se recomienda usar políticas RLS más restrictivas.
alter table eventos enable row level security;

create policy "Permitir insercion publica"
on eventos for insert
to anon
with check (true);

create policy "Permitir lectura publica"
on eventos for select
to anon
using (true);
